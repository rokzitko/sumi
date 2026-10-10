"""Health-test CLI checks using short, fixed-seed sequences."""

import argparse
import math
from pathlib import Path
import re
import subprocess


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run(sumi, args):
    return subprocess.run([str(sumi), "-t", *args], capture_output=True,
                          text=True, timeout=10)


def invalid(sumi, args, label, count=8):
    command = ["-ot", "0", *args]
    if count is not None:
        command = ["-c", str(count), *command]
    result = run(sumi, command)
    require(result.returncode == 1, f"{label}: exit={result.returncode}, stderr={result.stderr!r}")
    require(result.stdout == "", f"{label}: generated samples before rejecting options")
    require("Error:" in result.stderr, f"{label}: missing diagnostic")
    require("Health test: fail=" not in result.stderr, f"{label}: misleading health result")
    print(f"PASS: {label}")


def valid(sumi, blocksize, count, threshold, extra=()):
    args = ["-c", str(count), "-B", str(blocksize), "-ot", "0", *extra]
    if threshold is not None:
        args += ["-R", str(threshold)]
    result = run(sumi, args)
    require(result.returncode == 0, result.stderr)
    samples = [int(x) for x in result.stdout.split()]
    require(len(samples) == count, "incorrect generated sample count")
    limit = 0.0 if threshold is None else threshold
    failures = 0
    for start in range(0, count-blocksize+1, blocksize):
        block = samples[start:start+blocksize]
        rvar = 0.5*sum((b-a)**2 for a, b in zip(block, block[1:]))/(blocksize-1)
        failures += rvar < limit
    total = count // blocksize
    summary = re.search(r"Health test: fail=(\d+) pass=(\d+) "
                        r"fail_ratio=(\S+) pass_ratio=(\S+)", result.stderr)
    require(summary is not None, f"missing summary: {result.stderr!r}")
    failed, passed = map(int, summary.group(1, 2))
    require((failed, passed) == (failures, total-failures), "incorrect block classifications")
    for reported, expected in zip(map(float, summary.group(3, 4)),
                                  (failures/total, (total-failures)/total)):
        require(math.isfinite(reported) and math.isclose(reported, expected, rel_tol=5e-6,
                                                        abs_tol=1e-12), "incorrect ratio")
    print(f"PASS: B={blocksize}, count={count}, limit={limit}: fail={failed}, pass={passed}")
    return samples, failed, passed


def interrupted(sumi):
    # The pipe fills well before a million text samples can be generated.
    # Closing its read end therefore interrupts the run before the first block.
    process = subprocess.Popen([str(sumi), "-t", "-c", "0", "-B", "1048576",
                                "-R", "0.1", "-ot", "-1"],
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    process.stdout.close()
    process.stdout = None
    try:
        _, stderr = process.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
        process.communicate()
        raise AssertionError("generator did not stop after pipe closure")
    require(process.returncode == 1, f"interrupted run: exit={process.returncode}, {stderr!r}")
    require("no complete blocks" in stderr.lower(), f"missing incomplete-run diagnostic: {stderr!r}")
    require("Health test: fail=" not in stderr, "interrupted run reported undefined ratios")
    print("PASS: interrupted infinite run with no completed health blocks")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sumi", type=Path, default=Path(__file__).resolve().parent.parent / "sumi")
    sumi = parser.parse_args().sumi.resolve()

    # The first case is the original NaN-as-pass regression.
    invalid(sumi, ["-B", "1", "-R", "0.1"], "one-sample health block")
    for blocksize in ("-1", "-2", "2.5", "2x", "", "18446744073709551616"):
        invalid(sumi, ["-B", blocksize], f"invalid block size {blocksize!r}")
    invalid(sumi, ["-B"], "missing block-size value")
    for limit in ("nan", "inf", "-inf", "0.5x", "", "1e9999"):
        invalid(sumi, ["-B", "2", "-R", limit], f"invalid threshold {limit!r}")
    invalid(sumi, ["-B", "2", "-R"], "missing threshold value")
    invalid(sumi, ["-B", "0", "-R", "nan"], "non-finite threshold with testing disabled")
    invalid(sumi, ["-B", "16", "-R", "0.1"], "finite run shorter than its health block")
    invalid(sumi, ["-B", "16"], "default finite run shorter than its health block", count=None)

    for blocksize, count in ((2, 2), (2, 128), (3, 95), (8, 67), (8, 71)):
        valid(sumi, blocksize, count, 0.58)
    valid(sumi, 32, 3200, 0.58,
          ("-n", "12", "-ap", "0.132", "-aw", "0", "-b", "1024", "-mu", "0.5"))

    zero = ("-n", "12", "-ap", "0", "-aw", "0", "-b", "8")
    for limit in (None, 0.0, -1.0, 0.1):
        samples, failed, passed = valid(sumi, 2, 8, limit, zero)
        require(samples == [0]*8, "zero spectrum did not give constant integer samples")
        require((failed, passed) == ((4, 0) if limit == 0.1 else (0, 4)),
                "incorrect zero-variance boundary behavior")

    ramp = ("-n", "1", "-s", "1e-12", "-mu", "1", "-add")
    for limit in (0.5, math.nextafter(0.5, math.inf)):
        samples, failed, passed = valid(sumi, 2, 8, limit, ramp)
        require(samples == list(range(1, 9)), "incorrect ramp samples")
        require((failed, passed) == ((0, 4) if limit == 0.5 else (4, 0)),
                "incorrect equality boundary")

    for args in ([], ["-B", "0", "-R", "0.58"]):
        result = run(sumi, ["-c", "1", "-ot", "0", *args])
        require(result.returncode == 0 and len(result.stdout.split()) == 1,
                "disabled health test prevented generation")
        require("Health test:" not in result.stderr, "disabled test produced a summary")
    print("PASS: health testing disabled by default and by -B 0")
    interrupted(sumi)


if __name__ == "__main__":
    main()
