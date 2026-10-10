"""Check empirical window entropies against independently counted bit patterns."""

import argparse
from collections import Counter
import math
from pathlib import Path
import re
import subprocess


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def pack(bits):
    """Pack a short test sequence LSB first, padding its last byte with zeros."""
    data = bytearray((len(bits)+7)//8)
    for i, bit in enumerate(bits):
        data[i//8] |= int(bit) << (i % 8)
    return bytes(data)


def run(bitent, data, n, count=0, direction=1, table=True, verbose=False):
    args = [str(bitent), "-ws", "8", "-id", str(direction), "-n", str(n),
            "-c", str(count), "-x", "0"]
    if table:
        args.append("-t")
    if verbose:
        args.append("-v")
    return subprocess.run(args, input=data, capture_output=True, timeout=10)


def check(bitent, label, data, n, count=0, direction=1, table=True):
    order = range(8) if direction == 1 else range(7, -1, -1)
    bits = "".join(str((byte >> bit) & 1) for byte in data for bit in order)
    words = [bits[i:i+n] for i in range(len(bits)-n+1)]
    if count:
        words = words[:count]
    require(bool(words), "test case requires at least one complete window")
    counts = Counter(int(word, 2) for word in words)
    total = len(words)
    probabilities = [counts[i]/total for i in range(1 << n)]
    entropy = math.fsum(-p*math.log2(p) for p in probabilities if p > 0)
    min_entropy = -math.log2(max(probabilities))

    result = run(bitent, data, n, count, direction, table)
    require(result.returncode == 0, f"{label}: {result.stderr.decode()}")
    text = result.stdout.decode("ascii")
    observed_total = re.search(r"^nr=(\d+)", text, re.MULTILINE)
    require(observed_total is not None and int(observed_total[1]) == total,
            f"{label}: incorrect window count")
    summary = re.search(r"^H=(\S+) = (\S+) per bit\s+Hmin=(\S+) = (\S+) per bit$",
                        text, re.MULTILINE)
    require(summary is not None, f"{label}: missing entropy report")
    # -t selects 14 significant digits; otherwise the report uses four.
    tolerance = 5e-13 if table else 5e-4
    for actual, expected in zip(map(float, summary.groups()),
                                (entropy, entropy/n, min_entropy, min_entropy/n)):
        require(math.isfinite(actual), f"{label}: non-finite entropy {actual}")
        if expected == 0:
            require(actual == 0, f"{label}: expected exactly zero entropy")
        else:
            require(math.isclose(actual, expected, rel_tol=tolerance, abs_tol=5e-14),
                    f"{label}: entropy {actual}, expected {expected}")

    if table:
        rows = re.findall(r"^P\[([01]+) ~\s*(\d+)\]=\s*(\S+)\s+(\d+)$",
                          text, re.MULTILINE)
        require(len(rows) == 1 << n, f"{label}: missing probability-table bins")
        for i, (word, symbol, probability, observed) in enumerate(rows):
            require(len(word) == n and int(word, 2) == int(symbol) == i,
                    f"{label}: incorrect symbol ordering")
            require(int(observed) == counts[i], f"{label}: incorrect symbol count")
            require(math.isclose(float(probability), probabilities[i], rel_tol=tolerance,
                                 abs_tol=5e-14), f"{label}: incorrect probability")
    print(f"PASS: {label}: {total} windows, H={entropy:.12g}, Hmin={min_entropy:.12g}")


def insufficient(bitent, data, n, count=0, verbose=False):
    result = run(bitent, data, n, count=count, verbose=verbose)
    stderr = result.stderr.decode("ascii")
    require(result.returncode == 1, f"insufficient input: exit={result.returncode}")
    require(result.stdout == b"", "insufficient input produced an entropy report")
    require("Error:" in stderr and "complete" in stderr,
            f"insufficient input: missing diagnostic: {stderr!r}")
    print(f"PASS: reject {len(data)*8} input bits for a {n}-bit window")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bitent", type=Path,
                        default=Path(__file__).resolve().parent.parent / "bitent")
    bitent = parser.parse_args().bitent.resolve()

    # Existing valid behavior: every histogram bin is populated.
    check(bitent, "balanced bits", b"\x55", 1)
    check(bitent, "biased bits", b"\x03", 1)
    check(bitent, "all two-bit words", pack("00110"), 2, count=4)
    check(bitent, "all three-bit words", pack("0001011100"), 3, count=8)
    check(bitent, "full four-bit histogram", bytes(range(256)), 4)

    for n in (1, 3, 8):
        check(bitent, f"all zeros, n={n}", b"\x00"*4, n)
        check(bitent, f"all ones, n={n}", b"\xff"*4, n)
    check(bitent, "alternating three-bit words", b"\x55"*4, 3)
    check(bitent, "alternating five-bit words", b"\x55"*4, 5)
    check(bitent, "multiple populated and empty bins", b"\xf0\xf0", 4)
    check(bitent, "exactly one complete window", b"\x96", 8)
    check(bitent, "one-window limit", b"\xe8\x00", 3, count=1)
    check(bitent, "two-window limit", b"\xe8\x00", 3, count=2)
    check(bitent, "EOF before count limit, with complete windows", b"\x96", 3, count=100)
    check(bitent, "MSB-first input", b"\x03\xc1", 3, direction=2)
    check(bitent, "constant input without table", b"\x00", 1, table=False)
    check(bitent, "sparse histogram without table", b"\x96", 3, table=False)
    check(bitent, "biased bits without table", b"\x03", 1, table=False)

    insufficient(bitent, b"", 1)
    insufficient(bitent, b"", 3, count=1, verbose=True)
    insufficient(bitent, b"\x96", 9)
    insufficient(bitent, b"\x96\xa5", 17)


if __name__ == "__main__":
    main()
