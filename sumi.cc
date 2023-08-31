// Simple tool for generating noise and random bit sequences
// Rok Zitko, 2023

#include <iostream>
#include <iomanip>
#include <string>
#include <random>
#include <bitset>
#include <ctime>
#include <cassert>
#include <coroutine>
#include <math.h>
#include <unistd.h>

#include <complex>
#include <fftw3.h>

#include "common.h"
#include "misc.h"
#include "io.h"
#include "filter.h"

bool verbose;
auto & msg = std::cerr; // stream for diagnostic messages

enum class NoiseType {
  undefined = 0,
  white = 1,
  pink = 2
};

std::string name(NoiseType nt) {
  using enum NoiseType;
  switch (nt) {
  case undefined:
    return "undefined";
  case white:
    return "white";
  case pink:
    return "pink";
  default:
    assert(0);
  }
}

using Gen = Generator<double>;

Gen constant(double x)
{
  for (;;)
    co_yield x;
}

Gen white(std::default_random_engine &dre, double sigma)
{
  std::normal_distribution<double> di(0, sigma);
  for (;;)
    co_yield di(dre);
}

// Calculate total power
double calc_power(fftw_complex *d, uint64_t size)
{
  double pwr = 0.0;
  for (uint64_t i = 0; i < size; i++)
    pwr += d[i][0]*d[i][0] + d[i][1]*d[i][1];
  return pwr;
}

// Normalize total power to 1
void normalize(fftw_complex *d, uint64_t size)
{
  const double norm = 1.0/sqrt(calc_power(d, size));
  for (uint64_t i = 0; i < size; i++) {
    d[i][0] *= norm;
    d[i][1] *= norm;
  }
}

// Timmer Konig 1995 algorithm. Generate IFFT with Gaussian variation and power spectrum given by S
void timmer_konig_setup(fftw_complex *d, uint64_t size, std::function<double(uint64_t f)> S, std::default_random_engine &dre, bool norm = true)
{
  assert(size % 2 == 0);
  const uint64_t ny = size/2; // Nyquist frequency
  std::normal_distribution<double> di(0.0, 1.0);
  d[0][0] = d[0][1] = 0.0;
  for (uint64_t i = 1; i < ny; i++) {
    d[i][0] = di(dre) * sqrt(S(i)/2.0);
    d[i][1] = di(dre) * sqrt(S(i)/2.0);
    d[size-i][0] = d[i][0];
    d[size-i][1] = -d[i][1];
  }
  d[ny][0] = di(dre) * sqrt(S(ny)/2.0);
  d[ny][1] = 0.0;
  if (norm) normalize(d, size);
}

Gen pink(std::default_random_engine &dre, double sigma, uint64_t bs)
{
  // http://fftw.org/fftw3_doc/Complex-One_002dDimensional-DFTs.html
  // http://fftw.org/fftw3_doc/Complex-numbers.html
  fftw_complex *d;
  fftw_plan p;
  d = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * bs);
  p = fftw_plan_dft_1d(bs, d, d, FFTW_BACKWARD, FFTW_ESTIMATE); // in-place FFT
  auto S = [bs](uint64_t f) -> double { return 1.0/f; };
  for (;;) {
    timmer_konig_setup(d, bs, S, dre, true);
    fftw_execute(p);
    for (uint64_t i = 0; i < bs; i++)
      co_yield d[i][0]*sigma;
  }
}

int main(int argc, char *argv[])
{
  InputParser input(argc, argv);

  const NoiseType nt { input.exists("-n") ? std::stoi(input.get("-n")) : 1 };
  const double sigma { input.exists("-s") ? std::stod(input.get("-s")) : 1.0 }; // standard deviation of generated noise
  const bool additive { input.exists("-a") };
  const bool testing_mode { input.exists("-t") }; // use fixed seed for rng generator
  const uint64_t count { input.exists("-c") ? std::stoul(input.get("-c")) : 10 }; // number of random numbers generated, 0 = infinite
  const uint64_t bs { input.exists("-b") ? std::stoul(input.get("-b")) : 1<<10 }; // block size in FFT
  const IntFilter flt { input.exists("-f") ? std::stoi(input.get("-f")) : 0 }; // filtering for floating point to integer conversion (default=rounding)
  const OutputType ot { input.exists("-ot") ? std::stoi(input.get("-ot")) : -1 }; // floating, integer, bit, binary (default is floating point)
  const Direction od { input.exists("-od") ? std::stoi(input.get("-od")) : 1 }; // 1=lsb to msb, 2=msb to lsb
  const bool ow { input.exists("-ow") }; // byte swap for binary output (endianness change)
  verbose = input.exists("-v");

  if (verbose) {
    msg << "noise type nt=" << static_cast<int>(nt) << " [" << name(nt) << "]" << std::endl;
    msg << "sigma=" << sigma << std::endl;
    msg << "additive=" << std::boolalpha << additive << std::endl;
    show_with_logs(msg, "count", count, true);
    msg << "block size bs=" << bs << "=" << "2^" << log2(bs) << std::endl;
    msg << "integer filter flt=" << static_cast<int>(flt) << " [" << name(flt) << "]" << std::endl;
    msg << "output type ot=" << static_cast<int>(ot) << " " << name(ot) << std::endl;
    msg << "output direction od=" << static_cast<int>(od) << " [" << name(od) << "]" << std::endl;
    msg << "output endianness bswap=" << std::boolalpha << ow << std::endl;
  }

  const unsigned long seed = testing_mode ? 1234 : mix(clock(), time(NULL), getpid());
  std::default_random_engine dre(seed);

  Gen *gen;
  switch (nt) {
    case NoiseType::white:
      gen = new Gen(white(dre, sigma));
      break;
    case NoiseType::pink:
      gen = new Gen(pink(dre, sigma, bs));
      break;
    case NoiseType::undefined:
      std::cout << "Set the noise type using the -n switch." << std::endl;
      exit(1);
  }
  int ndx = 0;
  std::bitset<8> b8;
  std::bitset<16> b16;
  std::bitset<32> b32;
  std::bitset<64> b64;
  double total = 0.0;
  for (uint64_t i = 0; i < count || count == 0; i++) {
    double value = (*gen)();
    total += value;
    double x = additive ? total : value; // in additive mode, we analyse the accumulated sum rather than consecutive values
    int n = filter(x, flt);
    bool b = n%2 != 0; // handles -1, too!
    switch (ot) {
    case OutputType::floating:
      std::cout << x << std::endl;
      break;
    case OutputType::integer:
      std::cout << n << std::endl;
      break;
    case OutputType::lsb:
      std::cout << b << std::endl;
      break;
    case OutputType::lsb_8:
      b8[od == Direction::lm ? ndx : 7-ndx] = b;
      ndx++;
      if (ndx == 8) {
        writebs<uint8_t, 8>(b8, ow);
        ndx = 0;
      }
      break;
    case OutputType::lsb_16:
      b16[od == Direction::lm ? ndx : 15-ndx] = b;
      ndx++;
      if (ndx == 16) {
        writebs<uint16_t, 16>(b16, ow);
        ndx = 0;
      }
      break;
    case OutputType::lsb_32:
      b32[od == Direction::lm ? ndx : 31-ndx] = b;
      ndx++;
      if (ndx == 32) {
        writebs<uint32_t, 32>(b32, ow);
        ndx = 0;
      }
      break;
    case OutputType::lsb_64:
      b64[od == Direction::lm ? ndx : 63-ndx] = b;
      ndx++;
      if (ndx == 64) {
        writebs<uint64_t, 64>(b64, ow);
        ndx = 0;
      }
      break;
    case OutputType::none:
      break;
    }
  }
}
