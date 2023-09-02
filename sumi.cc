// Simple tool for generating noise and random bit sequences
// Rok Zitko, 2023

#include <iostream>
#include <iomanip>
#include <string>
#include <random>
#include <bitset>
#include <coroutine>
#include <ctime>
#include <cassert>
#include <csignal>
#include <csetjmp>
#include <math.h>
#include <unistd.h>

#include <complex>
#include <fftw3.h>

#include "version.h"
#include "common.h"
#include "misc.h"
#include "io.h"
#include "filter.h"
#include "stats.h"

bool verbose;
auto & msg = std::cerr; // stream for diagnostic messages

enum class NoiseType {
  undefined = 0,
  white = 1,
  pink = 2,
  pink_white = 3
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
  case pink_white:
    return "pink @ LF+white @ HF";
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

// S is the spectral function, if norm=true, the IFFT is normalied to total power 1, mult_factor is a prefactor for
// the generated variates.
Gen from_spectrum(std::default_random_engine &dre, std::function<double(uint64_t f)> S, uint64_t bs, bool norm, double mult_factor)
{
  // http://fftw.org/fftw3_doc/Complex-One_002dDimensional-DFTs.html
  // http://fftw.org/fftw3_doc/Complex-numbers.html
  fftw_complex *d;
  fftw_plan p;
  d = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * bs);
  p = fftw_plan_dft_1d(bs, d, d, FFTW_BACKWARD, FFTW_ESTIMATE); // in-place FFT
  for (;;) {
    timmer_konig_setup(d, bs, S, dre, norm);
    fftw_execute(p);
    for (uint64_t i = 0; i < bs; i++)
      co_yield d[i][0]*mult_factor;
  }
}

// Generate pink noise with 1/f spectrum at all frequencies.
Gen pink(std::default_random_engine &dre, double sigma, uint64_t bs)
{
  auto S = [bs](uint64_t f) -> double { return 1.0/f; };
  return from_spectrum(dre, S, bs, true, sigma);
}

// Generate pink noise at low frequencies, white noise at high frequencies. The cutoff frequency 'f_cutoff' is given 
// in frequency units such that the Nyquist frequency corresponds to 0.5. This means that for f_cutoff=0.5, the
// spectrum is purely pink, while for f_cutoff=0.25, the lower half is pink, the upper half is white, etc.
Gen pink_white(std::default_random_engine &dre, double sigma, double f_cutoff, uint64_t bs)
{
  const uint64_t f0 = bs*f_cutoff;
  auto S = [bs, f0](uint64_t f) -> double { return f <= f0 ? 1.0/f : 1.0/f0; };
  return from_spectrum(dre, S, bs, true, sigma);
}

// A SIGPIPE is sent to a process if it tried to write to a socket that had been shutdown for writing or isn't connected (anymore).
// https://stackoverflow.com/questions/1717991/throwing-an-exception-from-within-a-signal-handler
jmp_buf gBuffer;

void catch_signal(int signalNumber)
{
  longjmp(gBuffer, signalNumber);
}

class pipe_exception : public std::exception {
 public:
   const char *what () {
     return "Pipe close by the receiving end";
   }
};

int main(int argc, char *argv[])
{
  InputParser input(argc, argv);

  const NoiseType nt { input.exists("-n") ? std::stoi(input.get("-n")) : 1 };
  const double sigma { input.exists("-s") ? std::stod(input.get("-s")) : 1.0 }; // standard deviation of generated noise
  const double f_cutoff { input.exists("-cut") ? std::stod(input.get("-cut")) : 0.5 }; // cutoff frequency for mixed type spectra
  const bool additive { input.exists("-a") }; // if true, return values are accumulated variates
  const bool testing_mode { input.exists("-t") }; // use fixed seed for rng generator to generate reproducible sequences for testing
  const uint64_t count { input.exists("-c") ? std::stoul(input.get("-c")) : 10 }; // number of random numbers generated, 0 = infinite
  const uint64_t bs { input.exists("-b") ? std::stoul(input.get("-b")) : 1<<10 }; // block size in FFT
  const IntFilter flt { input.exists("-f") ? std::stoi(input.get("-f")) : 0 }; // filtering for floating point to integer conversion (default=rounding)
  const OutputType ot { input.exists("-ot") ? std::stoi(input.get("-ot")) : -1 }; // floating, integer, bit, binary (default is floating point)
  int ws = get_ws(ot);
  const Direction od { input.exists("-od") ? std::stoi(input.get("-od")) : 1 }; // 1=lsb to msb, 2=msb to lsb
  const bool ow { input.exists("-ow") }; // byte swap for binary output (endianness change)
  const bool stats { input.exists("-stats") };
  verbose = input.exists("-v");

  if (verbose) {
    msg << "sumi " << GIT_HASH << " " << __DATE__ << " " << __TIME__ << std::endl;
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
    case NoiseType::pink_white:
      gen = new Gen(pink_white(dre, sigma, f_cutoff, bs));
      break;
    case NoiseType::undefined:
      std::cout << "Set the noise type using the -n switch." << std::endl;
      exit(1);
  }
  int ndx = 0;
  std::bitset<64> bits(0);
  Stats stats_floating(msg, "floating");
  Stats stats_integer(msg, "integer");
  Stats stats_lsb(msg, "LSB");
  double total = 0.0;
  signal(SIGPIPE, catch_signal);
  try {
    if (setjmp(gBuffer) == 0) {
      for (uint64_t i = 0; i < count || count == 0; i++) {
        double value = (*gen)();
        total += value;
        double x = additive ? total : value; // in additive mode, we analyse the accumulated sum rather than consecutive values
        int n = filter(x, flt);
        bool b = n%2 != 0; // handles -1, too!
        if (stats) {
        stats_floating.add(x);
          stats_integer.add(n);
          stats_lsb.add(b);
        }
        using enum OutputType;
        switch (ot) {
        case floating:
          std::cout << x << std::endl;
          break;
        case integer:
          std::cout << n << std::endl;
          break;
        case lsb:
          std::cout << b << std::endl;
          break;
        case lsb_8:
        case lsb_16:
        case lsb_32:
        case lsb_64:
          bits[od == Direction::lm ? ndx : ws-1-ndx] = b;
          ndx++;
          if (ndx == ws) {
            write(bits, ws, ow);
            ndx = 0;
          }
          break;
        case none:
          break;
        }
      }
    } else {
      throw pipe_exception();
    }
  } catch (const pipe_exception &) {
    std::cerr << "Pipe closed." << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    exit(1);
  }
}
