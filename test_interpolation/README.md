# Interpolation regression checks

These checks exercise `from_spectrum_with_interpolation` directly and mode
`-n 120` through the compiled command-line program. They require a C++20 compiler,
FFTW3, Python 3, and NumPy. The test Makefile uses `pkg-config` to locate FFTW;
alternatively, set `FFTW` to the appropriate include and linker flags.

From the repository root, build the program and run the tests:

```sh
make sumi
make -C test_interpolation
```

For Apple Clang with FFTW available through `pkg-config`:

```sh
make sumi CXX=clang++ \
  CXXFLAGS="-std=c++20 -O3 -Werror -Wall -Wextra -Wshadow -Wpedantic $(pkg-config --cflags fftw3)" \
  FFTW="$(pkg-config --libs fftw3)"
make -C test_interpolation CXX=clang++
```

The focused suite generates 4,096,000 samples for each of three spectra; allow
roughly a minute, depending on the machine and its text-output throughput.
Samples are held in memory, with no saved data files. Peak memory is a few
hundred MB. Only `check.out` is created in this directory; `make clean` removes it.

## Deterministic checks

`check.cc` includes the implementation, renaming the CLI entry point so it can
call the generator directly. The explicit return in the CLI is also valid when
that entry point is compiled under a different name.

- Spectrum-callback counts check initialization and regeneration timing: two
  blocks at sample zero, then one every half-block.
- An independent blockwise overlap-add reference uses the unmodified
  `from_spectrum` block generator, with blocks beginning at `-BS/2, 0, BS/2, ...`.
  It uses the equivalent square-root Hann window. Sample-by-sample agreement
  checks startup, the weight profile, zero-weight replacement, and subsequent
  boundaries without duplicating the coroutine's indexing logic.
- Block sizes are 2, 6, 16, and 1024, including the smallest valid size and a
  non-power-of-two size. Both normalization settings, pink/white/mixed spectra,
  negative and zero multipliers, and a zero spectrum are covered. A zero spectrum
  is tested only with normalization disabled, since normalization to unit power
  is undefined for it.
- The comparison tolerance is `2e-11 * (1 + abs(reference))`, allowing rounding
  differences between the two equivalent window expressions.

Run just these checks with:

```sh
make -C test_interpolation check.out
./test_interpolation/check.out
```

## Phase statistics and spectra

`check.py` runs the actual CLI with `BS=1024`, 4,000 blocks, and `-t` (seed 1234).
The source parameters `(AP, AW)` are `(1, 0)` for pink, `(0, 1)` for white, and
`(0.1, 1)` for mixed noise. Random sequences can differ between C++ standard
libraries; all predictions concern ensemble moments, not a stored golden sequence.

For the FFT convention in `timmer_konig_setup`, write $N=\mathrm{BS}$ and define
the coefficient powers $P_k$ by

$$
P_0=0,\qquad P_k=P_{N-k}=S(k)\quad(1\leq k<N/2),\qquad
P_{N/2}=S(N/2)/2.
$$

The source variance and its lag-one covariance are

$$
V=\sum_{k=0}^{N-1}P_k,\qquad
C_1=\sum_{k=0}^{N-1}P_k\cos(2\pi k/N).
$$

The expected output variance is $V$ at every phase. The expected variance of a
first difference is $2[V-C_1\cos(\pi/N)]$, including at both regeneration phases.
The tests compare all 1,024 phase variances and first-difference variances with
these predictions, and explicitly compare each join with ordinary steps.

The 12% phase tolerance is approximately five standard errors for a Gaussian
variance estimated from 4,000 samples, with allowance for checking many phases
and correlations in first differences. The total-variance tolerance is 5%.
These tolerances distinguish sampling fluctuations from the original factor-of-two
phase modulation and pink-noise boundary spike.

Spectral checks use block-aligned rectangular periodograms, without subtracting
each segment's mean. One segment contains one full sine-weighted source block
and portions of two independent half-offset blocks. For $j=0,\ldots,N-1$, their
length-$N$ windows are $u_1(j)=\sin(\pi j/N)$,
$u_2(j)=\cos(\pi j/N)$ for $j<N/2$ and zero otherwise, and
$u_3(j)=-\cos(\pi j/N)$ for $j\geq N/2$ and zero otherwise.
With the unnormalized forward DFT $U_a=\operatorname{DFT}(u_a)$ and periodogram
$I_\ell=|\operatorname{DFT}(y)_\ell|^2/N$, the exact expectation is

$$
E[I_\ell]=\frac{1}{N}\sum_{k=0}^{N-1}P_k
\sum_{a=1}^{3}|U_a((\ell-k)\bmod N)|^2.
$$

The test averages measured periodograms over the 4,000 output blocks and checks
DC, the first nonzero bin, dyadic frequency bands, and Nyquist against this
prediction with a 12% tolerance. DC and Nyquist have larger sampling fluctuations
than multi-bin bands; adjacent segments are correlated. The spectral prediction
includes windowing and the existing DC/Nyquist conventions. The test also reports
the predicted departure from the unwindowed spectrum in bins 1 through 4, rather
than imposing an exact $1/f$ spectrum there.

Run just these checks, optionally specifying another compiled binary, with:

```sh
python3 test_interpolation/check.py --sumi ./sumi
```
