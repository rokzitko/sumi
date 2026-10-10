# Statistics checks

Run the focused C++ checks from the repository root:

```sh
make -C test_stats check
```

They require a C++20 compiler and use `stats.h` directly. FFTW and Python are
not needed. On macOS, the compiler can be selected with `CXX=clang++`.
The only generated file is `check_variance.out`.

`Stats` accumulates the mean and centered sum of squares using Welford's online
algorithm. The reported variance is the population variance

$$
V=\frac{1}{N}\sum_{i=1}^{N}(x_i-\bar{x})^2,
\qquad \bar{x}=\frac{1}{N}\sum_{i=1}^{N}x_i.
$$

The standard deviation is $\sqrt{V}$. Zero-lag correlation is one for finite
positive variance and undefined (NaN) for a constant series. Mean and variance are
undefined for an empty sample.

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

The existing `test1`, `test1a`, `test2`, and `test2a` scripts run longer noise
generation examples and save diagnostic output. The default `all` target runs
those examples; the `check` target always executes the focused checks.
