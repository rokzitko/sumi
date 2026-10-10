# Statistics checks

Run the focused C++ checks from the repository root:

```sh
make -C test_stats check
```

They require a C++20 compiler and use `stats.h` directly. FFTW and Python are
not needed. On macOS, the compiler can be selected with `CXX=clang++`.
The checks generate `check_variance.out` and `check_correlation.out`.

`Stats` accumulates the mean and centered sum of squares using Welford's online
algorithm. The reported variance is the population variance

$$
V=\frac{1}{N}\sum_{i=1}^{N}(x_i-\bar{x})^2,
\qquad \bar{x}=\frac{1}{N}\sum_{i=1}^{N}x_i.
$$

The standard deviation is $\sqrt{V}$. Zero-lag correlation is one for finite
positive variance and undefined (NaN) for a constant series. Mean and variance are
undefined for an empty sample.

The first-difference quantities `rvar` and `rdev` are undefined for fewer than
two samples. The second-difference quantities `avar` and `adev`, and the derived
`tvar` and `tdev`, are undefined for fewer than three samples. These cases are
reported as NaN and are included in the short-sample regression checks.

`check_variance.cc` compares the public text report with an independent,
two-pass reference calculated from deviations relative to the first sample.
This keeps the reference stable even when the mean is large. The comparisons
allow the report's six-significant-digit rounding (relative tolerance `5e-6`);
expected zeros must remain exactly zero.

Cases cover empty and short samples, known population variances, constants,
large positive and negative offsets, reversed sample order, reset/reuse, and
continued accumulation after reporting. The random cases use 100,000 normal
samples with seed 1234; the reference is calculated from the same samples, so
the checks do not depend on a platform-specific golden sequence. First- and
second-difference variances are also compared with direct calculations.

## Autocorrelation convention

For lag $k=1,2,3$ and $N>k$, the reported correlation is

$$
R_k=\frac{1}{(N-k)V}\sum_{i=k+1}^{N}(x_i-\bar{x})(x_{i-k}-\bar{x}),
$$

where $\bar{x}$ and $V$ are the full-series mean and population variance defined
above. Each lag is averaged over its $N-k$ available pairs. This normalization
permits magnitudes greater than one for finite records: for `{1,2,3,4}`, the
results are $R_1=1/3$, $R_2=-3/5$, and $R_3=-9/5$. Correlations are undefined
(NaN) when the variance is zero or no pairs exist at the requested lag.

The streaming calculation tracks the means $\bar{a}_k$, $\bar{b}_k$ of the
paired sequences $a_i=x_{i+k}$ and $b_i=x_i$, with $i=1,\ldots,N-k$, and their
centered cross-product sum $C_k=\sum_i(a_i-\bar{a}_k)(b_i-\bar{b}_k)$.
Welford covariance updates avoid subtracting large raw second moments. The
conversion to full-series centering is

$$
R_k=\frac{C_k/(N-k)+(\bar{a}_k-\bar{x})(\bar{b}_k-\bar{x})}{V}.
$$

The autocorrelation calculation stores three lag accumulators and the last
three samples, using constant memory.

`check_correlation.cc` compares all four reported coefficients with an
independent two-pass calculation over retained samples. It covers asymmetric
records, known normalization values, empty/short/constant sequences, positive
and negative offsets, reversal, negative scaling, and reset/reuse. Random cases
use seed 1234 and 4,096 samples, including white noise and the correlated
recurrence $x_i=0.8x_{i-1}+\epsilon_i$ with $x_0=0$ and independent unit-normal
$\epsilon_i$. The tolerance is `5e-6 * abs(reference) + 1e-12`, accounting for
report rounding and numerical roundoff near zero.

## Longer examples

The existing `test1`, `test1a`, `test2`, and `test2a` scripts run longer noise
generation examples and save diagnostic output. The default `all` target runs
those examples; the `check` target always executes the focused checks.
