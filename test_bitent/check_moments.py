"""Check symbol-label moments against exact rational moments of window counts."""

import argparse
from collections import Counter
from fractions import Fraction
import math
from pathlib import Path
import re
import subprocess


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def check(bitent, label, data, n, count=0, direction=1, table=False):
    order = range(8) if direction == 1 else range(7, -1, -1)
    bits = "".join(str((byte >> bit) & 1) for byte in data for bit in order)
    windows = len(bits)-n+1
    if count:
        windows = min(windows, count)
    require(windows > 0, "test needs a complete window")
    counts = Counter(int(bits[i:i+n], 2) for i in range(windows))
    # Raw moments are safe here because Python integer and Fraction arithmetic
    # are exact, even when subtracting nearly equal large quantities.
    mean = Fraction(sum(s*c for s, c in counts.items()), windows)
    variance = Fraction(sum(s*s*c for s, c in counts.items()), windows)-mean*mean
    expected = {"mean": float(mean), "var": float(variance), "stddev": math.sqrt(variance)}

    args = [str(bitent), "-ws", "8", "-id", str(direction), "-n", str(n),
            "-c", str(count), "-x", "0"]
    if table:
        args.append("-t")
    result = subprocess.run(args, input=data, capture_output=True, timeout=15)
    require(result.returncode == 0, result.stderr.decode())
    text = result.stdout.decode("ascii")
    observed = re.search(r"^nr=(\d+)", text, re.MULTILINE)
    require(observed is not None and int(observed[1]) == windows, "incorrect window count")
    tolerance = 5e-13 if table else 5e-4
    for name, reference in expected.items():
        field = re.search(rf"^{name}=(\S+)$", text, re.MULTILINE)
        require(field is not None, f"{label}: missing {name}")
        actual = float(field[1])
        require(math.isfinite(actual) and actual >= 0, f"{label}: invalid {name}={actual}")
        if reference == 0:
            require(actual == 0, f"{label}: {name} must be exactly zero")
        else:
            require(math.isclose(actual, reference, rel_tol=tolerance, abs_tol=5e-14),
                    f"{label}: {name}={actual}, expected {reference}")
    print(f"PASS: {label}: {windows} windows, exact variance={variance}")


def nearly_constant(n, count, zero_from_end=0):
    # Set exactly one zero near the end of an otherwise all-one processed
    # sequence. The count limit excludes padding in the final byte.
    length = n+count-1
    data = bytearray([255])*((length+7)//8)
    position = length-1-zero_from_end
    data[position//8] &= ~(1 << (position % 8))
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bitent", type=Path,
                        default=Path(__file__).resolve().parent.parent / "bitent")
    bitent = parser.parse_args().bitent.resolve()

    check(bitent, "balanced bits", b"\x55", 1, table=True)
    check(bitent, "biased bits", b"\x03", 1, table=True)
    check(bitent, "four equally frequent labels", b"\x0c", 2, count=4, table=True)
    check(bitent, "alternating words", b"\x55", 3, table=True)
    check(bitent, "multiple occupied labels", bytes(range(32)), 4, table=True)
    check(bitent, "constant zero", b"\x00\x00", 8, table=True)
    check(bitent, "constant upper label", b"\xff\xff", 8, table=True)
    check(bitent, "single LSB-first word", b"\x96", 8, count=1, table=True)
    check(bitent, "single MSB-first word", b"\x96", 8, count=1, direction=2, table=True)
    check(bitent, "large constant label", b"\xff\xff\xff", 20)

    count = 100000
    for n in (20, 8):
        data = nearly_constant(n, count)
        check(bitent, f"rare adjacent label, n={n}", data, n, count, table=n <= 8)
        complement = bytes(byte ^ 255 for byte in data)
        check(bitent, f"complemented rare label, n={n}", complement, n, count, table=n <= 8)
        check(bitent, f"two rare labels, n={n}", nearly_constant(n, count, 1),
              n, count, table=n <= 8)


if __name__ == "__main__":
    main()
