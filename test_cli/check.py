"""Argument syntax, numeric conversion, and enum checks for all three tools."""

import argparse
from pathlib import Path
import subprocess


VALUES = {
    "sumi": ("-n", "-ap", "-aw", "-s", "-cut", "-mu", "-c", "-b", "-f", "-ot", "-od", "-B", "-R"),
    "bitdump": ("-i", "-ws", "-id", "-c", "-ot", "-od"),
    "bitent": ("-i", "-ws", "-id", "-c", "-n", "-x"),
}
UNSIGNED = {"sumi": ("-c", "-b", "-B"), "bitdump": ("-c",), "bitent": ("-c", "-n", "-x")}
REALS = {"sumi": ("-ap", "-aw", "-s", "-cut", "-mu", "-R"), "bitdump": (), "bitent": ()}
INVALID_CHOICES = {
    "sumi": {"-n": (0, 4, 99), "-f": (-1, 4), "-ot": (-2, 2, 7), "-od": (0, 3)},
    "bitdump": {"-ws": (0, 7, 128), "-id": (0, 3), "-ot": (-1, 0, 2), "-od": (0, 3)},
    "bitent": {"-ws": (0, 7, 128), "-id": (0, 3)},
}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run(binary, args, data=b""):
    return subprocess.run([str(binary), *args], input=data, capture_output=True, timeout=10)


def rejected(binary, name, args, reason=None):
    base = []
    if name == "sumi":
        # Bound unexpected execution and suppress sample output on error paths.
        if "-c" not in args:
            base += ["-c", "1"]
        if "-ot" not in args:
            base += ["-ot", "99"]
    result = run(binary, [*base, *args])
    require(result.returncode == 1, f"{name} {args}: exit={result.returncode}, stderr={result.stderr!r}")
    require(result.stdout == b"", f"{name} {args}: output before validation")
    require(b"Error:" in result.stderr, f"{name} {args}: missing diagnostic")
    if reason is not None:
        require(reason in result.stderr, f"{name} {args}: wrong diagnostic {result.stderr!r}")


def check(binary, name):
    rejected(binary, name, ["-unknown"], b"Unknown option or argument")
    rejected(binary, name, ["unexpected-positional-argument"], b"Unknown option or argument")
    rejected(binary, name, ["-v", "unexpected-switch-value"], b"Unknown option or argument")
    for option in VALUES[name]:
        rejected(binary, name, [option], b"Missing value for " + option.encode())
        rejected(binary, name, [option, "-v"], b"Missing value for " + option.encode())
        value = "/dev/null" if option == "-i" else "1"
        rejected(binary, name, [option, value, option, value], b"may only be specified once")
        if option == "-i":
            continue
        for value in ("", "abc", "1junk"):
            rejected(binary, name, [option, value], option.encode())
        if option in REALS[name]:
            for value in ("nan", "inf", "-inf", "1e9999"):
                rejected(binary, name, [option, value], option.encode())
        else:
            rejected(binary, name, [option, "999999999999999999999999999999999999"], option.encode())
        if option in UNSIGNED[name]:
            rejected(binary, name, [option, "-1"], option.encode())
            rejected(binary, name, [option, "-18446744073709551615"], option.encode())
    for option, choices in INVALID_CHOICES[name].items():
        for value in choices:
            rejected(binary, name, [option, str(value)], option.encode())
            rejected(binary, name, ["-v", option, str(value)], option.encode())
    print(f"PASS: {name} rejects unknown, missing, repeated, malformed, and unsupported arguments")

    if name == "sumi":
        base = ["-n", "12", "-ap", "0", "-aw", "0", "-b", "8", "-c", "8", "-ot", "8"]
        flags, data = ["-v", "-t"], b""
        expected = b"\x00"
    elif name == "bitdump":
        base, flags, data = ["-c", "8"], ["-v", "-iw"], b"\x96"
        expected = b"0\n1\n1\n0\n1\n0\n0\n1\n"
    else:
        base, flags, data = ["-c", "8", "-x", "0"], ["-v", "-t"], b"\x96"
        expected = None
    once = run(binary, [*base, *flags], data)
    repeated = run(binary, [*base, *flags, *flags], data)
    require(once.returncode == repeated.returncode == 0, "valid switches rejected")
    require(once.stdout == repeated.stdout and once.stderr == repeated.stderr,
            "repeating switches changes behavior")
    if expected is not None:
        require(once.stdout == expected, "incorrect valid output")
    if name != "sumi":
        explicit_stdin = run(binary, [*base, *flags, "-i", ""], data)
        require(explicit_stdin.returncode == 0 and explicit_stdin.stdout == once.stdout,
                "empty filename no longer selects stdin")
    print(f"PASS: {name} valid input and repeated switches")


def main():
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    for name in VALUES:
        parser.add_argument(f"--{name}", type=Path, default=root/name)
    args = parser.parse_args()
    for name in VALUES:
        check(getattr(args, name).resolve(), name)


if __name__ == "__main__":
    main()
