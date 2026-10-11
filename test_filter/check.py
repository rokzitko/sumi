"""Check quantization errors and floating-only processing through the sumi CLI."""

import argparse
import math
from pathlib import Path
import struct
import subprocess


LOW = -(1 << (8*struct.calcsize("i")-1))
HIGH = -LOW-1


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run(sumi, value, mode=0, output=0, count=4, extra=()):
    # An unnormalized zero spectrum supplies exact constants, including ties,
    # without relying on a zero-width normal distribution.
    return subprocess.run([str(sumi), "-n", "12", "-ap", "0", "-aw", "0",
                           "-b", "8", "-t", "-c", str(count), "-mu", str(value),
                           "-f", str(mode), "-ot", str(output), *extra],
                          capture_output=True, timeout=10)


def invalid(sumi, value, mode=0, output=0, extra=()):
    result = run(sumi, value, mode, output, extra=extra)
    require(result.returncode == 1, f"invalid conversion: exit={result.returncode}, {result.stdout!r}")
    require(result.stdout == b"", "invalid first sample was emitted")
    require(b"Error:" in result.stderr, f"missing diagnostic: {result.stderr!r}")
    require(b"Health test: fail=" not in result.stderr, "invalid sample received a health verdict")


def integer(sumi, value, mode, expected):
    result = run(sumi, value, mode)
    require(result.returncode == 0, result.stderr.decode())
    require([int(x) for x in result.stdout.split()] == [expected]*4,
            f"incorrect integer result: value={value}, mode={mode}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sumi", type=Path, default=Path(__file__).resolve().parent.parent / "sumi")
    sumi = parser.parse_args().sumi.resolve()

    # Original regression: the out-of-range even value was emitted as INT_MAX.
    invalid(sumi, HIGH+1)
    for mode in range(4):
        for value in (LOW-1, HIGH+1):
            for output in (0, 1, 8, 16, 32, 64, 99):
                invalid(sumi, value, mode, output)
        integer(sumi, LOW, mode, LOW)
        integer(sumi, HIGH, mode, HIGH)
    print("PASS: integer boundaries for all filters and integer/bit/no-output modes")

    for mode in (0, 1, 3):
        integer(sumi, HIGH+0.25, mode, HIGH)
    for mode in (0, 2, 3):
        integer(sumi, LOW-0.25, mode, LOW)
    invalid(sumi, HIGH+0.25, mode=2)
    invalid(sumi, LOW-0.25, mode=1)
    for value, expected in ((-1.5, (-2, -2, -1, -1)), (-0.5, (-1, -1, 0, 0)),
                            (0.5, (1, 0, 1, 0)), (1.5, (2, 1, 2, 1))):
        for mode, rounded in enumerate(expected):
            integer(sumi, value, mode, rounded)
    print("PASS: check the filtered result, including ties away from zero")

    for value in (LOW, HIGH, -2, -1):
        for output in (1, 8, 16, 32, 64):
            result = run(sumi, value, output=output, count=64)
            require(result.returncode == 0, result.stderr.decode())
            bit = int(value % 2 != 0)
            expected = (f"{bit}\n"*64).encode() if output == 1 else bytes([255*bit])*8
            require(result.stdout == expected, "incorrect signed parity or bit packing")
    print("PASS: negative and boundary-value bit output")

    for value in (LOW-1, HIGH+1, -1e20, 1e20, 1e308):
        result = run(sumi, value, output=-1)
        require(result.returncode == 0, result.stderr.decode())
        samples = [float(x) for x in result.stdout.split()]
        require(len(samples) == 4 and all(math.isfinite(x) and
                math.isclose(x, value, rel_tol=5e-6) for x in samples), "incorrect floating-only samples")
    result = run(sumi, 1e20, output=-1, extra=("-add",))
    require(result.returncode == 0, result.stderr.decode())
    require([float(x) for x in result.stdout.split()] == [1e20, 2e20, 3e20, 4e20],
            "floating-only accumulation was quantized")
    for extra in (("-carry",), ("-stats",), ("-B", "2", "-R", "0.1")):
        invalid(sumi, HIGH+1, output=-1, extra=extra)
    print("PASS: floating-only bypass and integer-dependent processing")

    for value in ("nan", "inf", "-inf"):
        for output in (-1, 0, 1, 8, 16, 32, 64, 99):
            invalid(sumi, value, output=output)
    result = run(sumi, 1e308, output=-1, extra=("-add",))
    require(result.returncode == 1 and b"Error:" in result.stderr, "unreported floating overflow")
    require([float(x) for x in result.stdout.split()] == [1e308], "non-finite accumulated sample emitted")
    result = run(sumi, HIGH, output=0, extra=("-add",))
    require(result.returncode == 1 and b"Error:" in result.stderr, "unreported integer overflow")
    require([int(x) for x in result.stdout.split()] == [HIGH], "out-of-range accumulated sample emitted")
    for value, mode, expected in ((HIGH+0.25, 1, HIGH), (LOW-0.25, 2, LOW)):
        result = run(sumi, value, mode, extra=("-carry",))
        require(result.returncode == 1 and b"Error:" in result.stderr, "unreported carry overflow")
        require([int(x) for x in result.stdout.split()] == [expected]*3,
                "incorrect carry sequence before the range error")
    print("PASS: non-finite samples and overflow stop before emitting the invalid sample")

    for mode in (-1, 4, 99):
        for output in (-1, 0, 99):
            invalid(sumi, 0.0, mode, output)
    print("PASS: invalid filter codes are rejected even for floating-only output")


if __name__ == "__main__":
    main()
