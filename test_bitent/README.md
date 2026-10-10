# Window-entropy regression checks

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

The Python check supplies packed bytes through stdin and captures the report
in memory. It creates no data files. The existing default `all` target runs
the longer noise-generation examples.

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

To test a particular executable:

```sh
python3 test_bitent/check_entropy.py --bitent ./bitent
```
