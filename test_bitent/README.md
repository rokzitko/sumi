# Entropy and run-count regression checks

Build `bitent`, then run the focused checks from the repository root:

```sh
make bitent
make -C test_bitent check
```

The checks require a C++20 compiler and Python 3.7 or later (standard library
only). FFTW is not needed to build or test `bitent`. For Apple Clang:

```sh
make bitent CXX=clang++ \
  CXXFLAGS="-std=c++20 -O3 -Werror -Wall -Wextra -Wshadow -Wpedantic"
make -C test_bitent check
```

The Python checks supply packed bytes through stdin and capture reports in
memory. They create no data files; the C++ sizing check creates `check_sizes.out`.
The existing default `all` target runs the longer noise-generation examples.

## Definitions

For a window length of $n$ bits, let $M$ be the number of complete sliding
windows counted and $c_s$ the count of word $s$, with $s=0,\ldots,2^n-1$.
For $M>0$, the empirical probabilities and entropies are

$$
p_s=\frac{c_s}{M},\qquad
H=-\sum_{s:p_s>0}p_s\log_2 p_s,\qquad
H_{\min}=-\log_2\max_s p_s.
$$

The per-bit values on the window-entropy summary line are $H/n$ and
$H_{\min}/n$. Unobserved words remain in the probability table with probability
zero and contribute zero to Shannon entropy. All-zero or all-one bit sequences,
and a single observed window, have zero empirical window entropy.

With no complete window ($M=0$), no empirical distribution is available.
`bitent` reports an error on stderr and returns status 1 before printing the
probability or entropy report. The `-c` option is an upper limit on counted
windows: EOF with at least one complete window still gives an estimate based
on the available windows.

## Run-table definitions

The run table uses all $L$ processed bits, including the first $n-1$ bits needed
to fill the histogram window. For a requested length $j\leq L$, there are
$L-j+1$ overlapping windows. `0:nr` and `1:nr` count the all-zero and all-one
windows among them, and their reported probabilities use this same denominator.
Other words contribute to the denominator. For a fixed processed bit sequence,
the run table is independent of `-n`.

The quantity printed as `Hmin` in the length-$j$ row is

$$
h_j=-\log_2\max\{p_{0,j},p_{1,j}\},
$$

with per-bit value $h_j/j$. If $j>L$, no windows have been observed and the
probabilities and entropy fields are NaN. If windows have been observed but
neither uniform word occurs, $h_j$ is positive infinity. The final summary is
the minimum of the full-histogram $H_{\min}/n$ and the run-based $h_j/j$ values
for which at least one uniform-word count reaches 1,000.

With a finite `-c C`, at most $n+C-1$ bits are processed. Changing `-n` with the
same count limit can therefore change the processed sequence and its run counts.

## Size and allocation checks

Histogram window lengths are restricted to `1`–`30`, and bin counts are formed
with an unsigned 64-bit shift. `-x` accepts nonnegative integers whose `x+1`
array lengths fit both the counter and probability vectors. The check is made
before addition, and loop indices are 64-bit. Malformed or out-of-range sizing
options produce an error before input processing.

The two histogram arrays require about $16\cdot 2^n$ bytes, and the eight run
arrays about $64(x+1)$ bytes, excluding container overhead. The numeric bounds
do not impose a RAM budget; allocation is subject to available memory. C++
allocation and length exceptions are reported with exit status 1.

## Coverage

`check_entropy.py` uses `-x 0` to select the histogram-based entropy report.
An independent Python reference slices the decoded bit sequence into windows,
counts their occurrences, and calculates the entropies from those counts.

The cases cover full and sparse histograms, all-zero and all-one inputs,
balanced and biased bits, overlapping windows, one-window samples, count limits,
both byte bit orders, and empty/insufficient input. The checks verify symbol
ordering, counts, zero-probability table entries, window counts, and both entropy
values and their per-bit normalization.

Comparisons allow report rounding: relative tolerance `5e-13` with the
14-significant-digit table output, or `5e-4` without the table (four significant
digits), plus absolute tolerance `5e-14`. Expected zero entropies must be exactly
zero. All inputs are deterministic; no random sampling is used in these checks.

`check_runs.py` independently enumerates uniform words in all processed bits.
It checks the initial prefix, invariance across histogram widths, limited reads,
bit order, unobserved lengths, and the final-summary occurrence threshold. It
also checks rejection of invalid `-n` and `-x` arguments before allocation.

`check_sizes.cc` includes the implementation with its CLI entry point renamed.
It verifies the sizing arithmetic at `n=30`, at vector-capacity boundaries, and
at integer limits without allocating the corresponding arrays. A single-shot
allocation failure in this test executable verifies that the CLI returns a
diagnostic and status 1. The explicit return in the implementation also permits
compiling its entry point under the test's alternative name.

To run the Python checks against a particular executable:

```sh
python3 test_bitent/check_entropy.py --bitent ./bitent
python3 test_bitent/check_runs.py --bitent ./bitent
```
