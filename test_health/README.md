# Health-test regression checks

Build `sumi`, then run the focused checks from the repository root:

```sh
make sumi
make -C test_health check
```

Requirements are a C++20 compiler, FFTW3, and Python 3.9 or later (standard library only).
The test Makefile uses `pkg-config` for FFTW include/link flags; these can also
be supplied through `FFTW`. For Apple Clang:

```sh
make sumi CXX=clang++ \
  CXXFLAGS="-std=c++20 -O3 -Werror -Wall -Wextra -Wshadow -Wpedantic $(pkg-config --cflags fftw3)" \
  FFTW="$(pkg-config --libs fftw3)"
make -C test_health check CXX=clang++
```

The suite uses short runs and keeps their output in memory. It creates only
`check.out`, and does not overwrite the saved `.dat` examples. The existing
default `all` target runs the longer historical examples.

## Checks

- `check.cc` includes the production implementation with the CLI entry point
  renamed. It tests threshold equality, values above/below the threshold, and
  rejection of NaN and infinities. It also exercises undefined/non-finite
  statistics obtained from `Stats`. A non-finite statistic raises a calculation
  error rather than contributing to either simulated block count.
- `check.py` runs the actual CLI with seed 1234. It checks invalid/missing block
  sizes and thresholds, numeric overflow and trailing junk, one-sample blocks,
  and finite runs shorter than one health block. A specified non-finite `-R`
  is rejected even when testing is disabled.
- For valid runs, the Python check reads the emitted integer sequence and
  calculates the first-difference variance of each complete block directly.
  This verifies classification, reset boundaries, pass/fail ratios, and the
  exclusion of incomplete final blocks without depending on a platform-specific
  random sequence. Ratio comparisons allow six-significant-digit report rounding.
- Zero-spectrum runs test constant integer sequences and the default zero
  threshold. A unit-increment integer sequence tests exact equality at `-R 0.5`
  and a threshold immediately above it. Ordinary failed blocks retain a successful
  simulation exit status.
- A subprocess with `-c 0` has its output pipe closed before the first health
  block can complete. The expected result is a nonzero exit status and an
  explicit "no complete blocks" diagnostic, without pass/fail ratios. The check
  has a timeout and terminates the child if it fails to stop.

Run just the CLI checks against a specified executable with:

```sh
python3 test_health/check.py --sumi ./sumi
```

The corresponding sample-size checks on the statistics themselves are part of
`make -C test_stats check`. The health-test observable, option defaults, and
exit-status conventions are documented in the root README.
