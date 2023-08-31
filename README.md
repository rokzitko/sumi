# sumi, noise generation tool

This tool generates a time series of a noisy process. In particular, it may be used for generating flicker (1/f) noise.
Random numbers with power-law spectrum are generated using the Timmer-Koenig algorithm.

J. Timmer, M. Koenig, "On generating power law noise", Astronomy and astrophysics (1995).

## Command line options

Syntax: `sumi [arguments]`

- `-n 1`: white noise [default]
- `-n 2`: pink noise (flicker, 1/f)

- `-s SIGMA`: standard deviation [default is 1.0]

- `-t`: use a fixed seed value for the random number generator (to generate reproducible sequences)

- `-c`: number of generated elements in the time series [default is 0 which stands for infinite length]

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

