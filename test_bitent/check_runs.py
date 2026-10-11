"""Check uniform-word counts over all consumed bits and reject invalid sizes."""

import argparse
from collections import Counter
import math
from pathlib import Path
import re
import subprocess


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def close(actual, expected):
    if math.isnan(expected):
        return math.isnan(actual)
    if math.isinf(expected):
        return actual == expected
    return math.isfinite(actual) and math.isclose(actual, expected, rel_tol=5e-13,
                                                 abs_tol=5e-14)


def check(bitent, data, n, maxlen, count=0, direction=1):
    order = range(8) if direction == 1 else range(7, -1, -1)
    bits = "".join(str((byte >> bit) & 1) for byte in data for bit in order)
    if count:
        bits = bits[:n+count-1]
    words = Counter(bits[i:i+n] for i in range(len(bits)-n+1))
    require(bool(words), "test needs a complete histogram window")
    expected_final = -math.log2(max(words.values())/sum(words.values()))/n

    args = [str(bitent), "-ws", "8", "-id", str(direction), "-n", str(n),
            "-x", str(maxlen), "-c", str(count), "-t"]
    result = subprocess.run(args, input=data, capture_output=True, timeout=10)
    require(result.returncode == 0, result.stderr.decode())
    text = result.stdout.decode("ascii")
    rows = re.findall(r"^len=\s*(\d+)\s+0:nr=(\d+),p=(\S+)\s+1:nr=(\d+),p=(\S+)"
                      r"\s+Hmin=\s*(\S+)\s+=\s+(\S+) per bit[ \t]*$", text, re.MULTILINE)
    require(len(rows) == maxlen, "incorrect number of run-table rows")
    for j, row in enumerate(rows, start=1):
        length, observed0, p0, observed1, p1, hmin, perbit = row
        windows = [bits[i:i+j] for i in range(max(0, len(bits)-j+1))]
        zeros = windows.count("0"*j)
        ones = windows.count("1"*j)
        require((int(length), int(observed0), int(observed1)) == (j, zeros, ones),
                f"n={n}, length={j}: counts {(observed0, observed1)}, expected {(zeros, ones)}")
        if windows:
            expected0, expected1 = zeros/len(windows), ones/len(windows)
            maximum = max(expected0, expected1)
            expected_h = -math.log2(maximum) if maximum > 0 else math.inf
        else:
            expected0 = expected1 = expected_h = math.nan
        for actual, expected in zip(map(float, (p0, p1, hmin, perbit)),
                                    (expected0, expected1, expected_h, expected_h/j)):
            require(close(actual, expected),
                    f"n={n}, length={j}: statistic {actual}, expected {expected}")
        if zeros >= 1000 or ones >= 1000:
            expected_final = min(expected_final, expected_h/j)

    final = re.search(r"^Hmin = (\S+) per bit$", text, re.MULTILINE)
    require(final is not None and close(float(final[1]), expected_final),
            "incorrect final min-entropy summary")
    print(f"PASS: {len(bits)} consumed bits, n={n}, x={maxlen}, count={count}, id={direction}")
    return rows


def invalid(bitent, option, value):
    args = [str(bitent), "-x", "0"] if option == "-n" else [str(bitent), "-n", "1"]
    args.append(option)
    if value is not None:
        args.append(value)
    result = subprocess.run(args, input=b"\xff", capture_output=True, timeout=10)
    require(result.returncode == 1 and result.stdout == b"",
            f"invalid {option} {value!r}: exit={result.returncode}")
    require(b"Error:" in result.stderr, "missing size diagnostic")
    print(f"PASS: reject {option} {value!r}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bitent", type=Path,
                        default=Path(__file__).resolve().parent.parent / "bitent")
    bitent = parser.parse_args().bitent.resolve()

    # First establish the working n=1 case, then reproduce the lost prefix at n=8.
    expected = check(bitent, b"\xff", 1, 8)
    require(check(bitent, b"\xff", 8, 8) == expected, "run table depends on -n")
    check(bitent, b"\x00", 8, 8)

    for data in (b"\x55\xaa", b"\x13\xb0", b"\x00\xff", bytes(range(32))):
        expected = check(bitent, data, 1, 32)
        for n in (2, 3, 8):
            require(check(bitent, data, n, 32) == expected, "run table depends on -n")
    for count in (1, 2, 3):
        check(bitent, b"\x13\xb0", 3, 12, count=count)
    check(bitent, b"\x13\xb0", 3, 12, count=3, direction=2)
    check(bitent, b"\x13\xb0", 3, 0)
    check(bitent, b"\xff", 8, 12, count=100)

    # Enough uniform words to exercise the >=1000 occurrence rule in the final
    # summary. Unequal zero/one runs make a missing initial zero prefix visible.
    data = (b"\x00"*9+b"\xff"*8)*64
    expected = check(bitent, data, 1, 32)
    for n in (3, 8):
        require(check(bitent, data, n, 32) == expected, "long-run table depends on -n")

    # Exactly 1000 zero words at length 25, and 999 at length 26.
    data = b"\x00"*128+b"\xff"*64
    expected = check(bitent, data, 1, 32)
    require(check(bitent, data, 8, 32) == expected, "threshold-boundary run table depends on -n")

    # These must fail before allocating the histogram or run arrays.
    for n in ("0", "31", "32", "63", "64", "-1", "-18446744073709551615",
              "3x", "1.5", "", None, "18446744073709551616"):
        invalid(bitent, "-n", n)
    for x in ("-1", "-18446744073709551615", "2x", "2.5", "", None,
              "18446744073709551615", "18446744073709551616"):
        invalid(bitent, "-x", x)


if __name__ == "__main__":
    main()
