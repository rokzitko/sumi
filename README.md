# sumi, noise generation tool

This tool generates a time series of a noisy process. In particular, it may be used for generating flicker (1/f) noise.
Random numbers with power-law spectrum are generated using the Timmer-Koenig algorithm.

J. Timmer, M. Koenig, "On generating power law noise", Astronomy and astrophysics (1995).

## Command line options

Syntax: `sumi [arguments]`

- `-n 1`: white noise [default]
- `-n 2`: pink noise (flicker, 1/f)
- `-n 120`: pink-plus-white noise with constant-power, half-block overlap (see below)

- `-s SIGMA`: standard deviation [default is 1.0]

- `-t`: use a fixed seed value for the random number generator (to generate reproducible sequences)

- `-c COUNT`: number of generated elements in the time series [default is `10`; `0` means infinite length]

- `-b BS`: sets the block size for performing Fourier transforms

- `-f 0`: conversion of floating point to integer: round-off
- `-f 1`: floor
- `-f 2`: ceiling
- `-f 3`: truncation

- `-ot -1`: output: floating-point in ASCII
- `-ot 0`: integer in ASCII
- `-ot 1`: lowest-significant bit (lsb) in ASCII
- `-ot 8`: binary packed with consecutive bits, 8-bit words
- `-ot 16`: binary packed with consecutive bits, 16-bit words
- `-ot 32`: binary packed with consecutive bits, 32-bit words
- `-ot 64`: binary packed with consecutive bits, 64-bit words

- `-od 1`: LSB to MSB bit order within words (default)
- `-od 2`: MSB to LSB bit order within words

- `-ow`: endianness conversion (e.g. little-endian to big-endian) for byte order in words

- `-v`: increase verbosity (directed to stderr)

- `-B B`: health-test block size; `0` disables testing [default], otherwise at least `2`
- `-R LIMIT`: finite lower threshold for the health-test first-difference variance [default `0`]

## Overlapped pink-plus-white noise

Mode `-n 120` combines independent FFT blocks staggered by half a block. Pass
nonnegative amplitudes with `-ap AP` and `-aw AW`, and an even block size `-b BS`
of at least 2. The source blocks use spectral weights
$S(k)=\mathrm{AP}/k+\mathrm{AW}/\mathrm{BS}$, where $k=1,\ldots,\mathrm{BS}/2$
is the FFT frequency index; the DC coefficient is zero. For example:

```sh
./sumi -n 120 -ap 0.1 -aw 1 -b 1024 -c 4096 -t
```

Set `-aw 0` for a purely pink source spectrum or `-ap 0` for a purely white
source spectrum. In this mode, `-ap` and `-aw` set the amplitudes; `-s` does not
rescale them.

Each buffer is weighted by $w(i)=\sin(\pi i/\mathrm{BS})$ using its own sample
index. Its weight is zero when it is regenerated. The two squared weights sum
to one, so the ensemble variance is independent of position within the block
and equals that of an individual source block. This is an ensemble statement,
not a normalization of each output block.

Windowing modifies temporal correlations and the spectrum, particularly near
and below the block frequency $1/\mathrm{BS}$ (sample spacing is one). The
overlapped stream does not have an exact, unmodified $1/f$ spectrum at all
frequencies. Reproducible timing, normalization, phase-statistics, and spectral
checks are described in [test_interpolation/README.md](test_interpolation/README.md).

## Health-test simulation

With `-B B`, complete, non-overlapping blocks of the integer samples are tested
using

$$
r_{\mathrm{var}}=\frac{1}{2(B-1)}\sum_{j=2}^{B}(n_j-n_{j-1})^2,
$$

where $n_j$ is the integer sample after the selected filtering and generation
options. A block fails when $r_{\mathrm{var}}<\mathrm{LIMIT}$; equality passes.
The test uses these integer samples regardless of the output type. For example:

```sh
./sumi -n 1 -s 1 -c 10000 -B 1000 -R 0.5 -t -ot 99
```

Here `-ot 99` suppresses sample output. The summary on stderr counts only complete
blocks. An incomplete final block does not contribute to the pass/fail counts.
Failed blocks are simulation results and do not themselves cause a nonzero
process exit status.

Malformed health-test options and non-finite thresholds are rejected before
generation. When testing is enabled, a finite `-c` must be at least `-B`;
`-c 0` permits an unlimited run. If the output pipe closes before any complete
block has been evaluated, the program reports an error without pass/fail ratios.
A non-finite test statistic is also a calculation error. These errors produce
a nonzero exit status. See [test_health/README.md](test_health/README.md) for
the focused regression checks.

## Note on floating-point to integer conversion

See (https://cplusplus.com/reference/cmath/trunc/) for details on rounding of floating point
numbers.

Note the following:

```
value	round	floor	ceil	trunc
-----	-----	-----	----	-----
2.1 	2.0 	2.0 	3.0 	2.0
1.1 	1.0 	1.0 	2.0 	1.0
0.1 	0.0 	0.0 	1.0 	0.0
-0.9 	-1.0 	-1.0 	-0.0 	-0.0
-1.9 	-2.0 	-2.0 	-1.0 	-1.0
```

Truncation does not maintain the even/odd sequence across the zero point.

## bitdump

`bitdump` is a tool for dumping time series of bit. It can also perform various bit-order (LSB to MSB vs. MSB to LSB), wordsize (between 8, 16, 32 and 64 bits) and byte-order (endianness) conversions.

## bitent

`bitent` is a tool for calculating Shannon and minimal entropy for a bit series.

Input is packed binary data from stdin, or from a file selected with `-i FILE`.
The default input word size is 8 bits, read least-significant bit first.
`-n N` sets the sliding-window length in bits [`1`–`30`, default `1`]; `-c COUNT` limits
the number of complete windows counted [default `0`, meaning until EOF].
`-t` prints the full probability table, including unobserved words with zero
probability. `-x X` reports all-zero/all-one window counts for lengths `1` through
`X` [default `32`]; `-x 0` omits this report. Run counts include every processed
bit, including the prefix before the first histogram window is complete. For example:

```sh
./sumi -n 1 -s 1 -t -c 4096 -ot 8 | ./bitent -ws 8 -n 3 -x 0 -t
```

Shannon entropy uses the convention $0\log_2 0=0$. If no complete window is
available, the program reports an error and exits with status 1 without a
probability or entropy report. Otherwise, it calculates empirical window
entropies from the windows actually observed, even if EOF occurs before the
`-c` limit. See [test_bitent/README.md](test_bitent/README.md) for the definitions
and regression checks.

Histogram storage grows as $16\cdot 2^N$ bytes for the count and probability
arrays: about 16 MiB at `-n 20`, 256 MiB at `-n 24`, and 16 GiB at `-n 30`.
Run arrays require about $64(X+1)$ additional bytes. Sizes are checked before
allocation; C++ allocation failures are reported as errors with exit status 1.

Sample outout:

```
nr=200000000000=10^11.3=2^37.54
P[000 ~    0]=0.12514958368         25029916736
P[001 ~    1]=0.12499761538         24999523076
P[010 ~    2]=0.124855502795        24971100559
P[011 ~    3]=0.124996685525        24999337105
P[100 ~    4]=0.124997615445        24999523089
P[101 ~    5]=0.1248545729          24970914580
P[110 ~    6]=0.124996685555        24999337111
P[111 ~    7]=0.12515173872         25030347744
mean=3.500004288195
var=5.2530018257816
stddev=2.2919428059578
Pmin=0.1248545729
Pmax=0.12515173872 Hmin=2.9982497605108
H=2.9999994952889 = 0.99999983176297 per bit
Hmin=2.9982497605108 = 0.99941658683692 per bit
```

## Requirements

- C++20 compiler (all testing done with `GCC 11.3.0`)
- `FFTW3` library

## Contact information:

These codes were created in the scope of a Target research programme (Ciljni raziskovalni programi, CRP)
V1-2119 "Cryptographically secure random number generator", funded by UVTP and ARIS, Slovenia.

 [Project home page](https://www.ijs.si/ijsw/ARRSProjekti/2021/Kriptografsko%20varen%20generator%20naklju%C4%8Dnih%20%C5%A1trevil)

```
   Rok Zitko
   "Jozef Stefan" Institute
   F1 - Theoretical physics
   Jamova 39
   SI-1000 Ljubljana
   Slovenia

   rok.zitko@ijs.si
```

