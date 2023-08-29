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

`bitdump` is a tool for dumping time series of bit. It can also perform various bit-order (LSB to MSB vs. MSB to LSB), wordsize and byte-order (endianness) conversions.

## bitent

`bitent` is a tool for calculating Shannon and minimal entropy for a bit series.

## Requirements

- C++20 compiler (all testing done with `GCC 11.3.0`)
- `FFTW3` library

## Contact information:

These codes were created in the scope of a Target research programme (Ciljni raziskovalni programi, CRP) 
V1-2119 "Cryptographically secure random number generator", funded by UVTP and ARRS, Slovenia.

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

