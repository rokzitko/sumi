// Calculate Shannon and minimal entropy for a bit series.
// Rok Zitko, 2023

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <random>
#include <bitset>
#include <stdexcept>
#include <new>
#include <algorithm>
#include <limits>
#include <ctime>
#include <cassert>
#include <coroutine>
#include <math.h>
#include <limits>
#include <unistd.h>

#include <complex>

#include "version.h"
#include "misc.h"
#include "io.h"
#include "readbits.h"

bool verbose, debug;
auto & msg = std::cerr; // stream for diagnostic messages

using vc = std::vector<uint64_t>;

std::string bits(uint64_t n, int len) {
  const std::bitset<64> bs(n);
  std::string s;
  for (int i = 0; i < len; i++)
    s += bs[len-1-i] ? "1" : "0";
  return s;
}

uint64_t histogram_bins(uint64_t n)
{
  if (n < 1 || n > 30)
    throw std::invalid_argument("-n must be between 1 and 30.");
  return uint64_t{1} << n;
}

std::size_t run_array_size(uint64_t maxlen)
{
  const auto capacity = std::min(vc{}.max_size(), std::vector<double>{}.max_size());
  // Check before adding one, so an excessive -x cannot wrap to a zero-sized array.
  if (maxlen >= capacity)
    throw std::length_error("-x is too large for the run arrays.");
  return static_cast<std::size_t>(maxlen)+1;
}

int main(int argc, char *argv[]) try
{
  InputParser input(argc, argv);
  input.validate_options({"-i", "-ws", "-id", "-c", "-n", "-x"}, {"-iw", "-v", "-d", "-t"});

  const std::string filename { input.exists("-i") ? input.get("-i") : "" };
  const int ws { input.get_choice("-ws", 8, {8, 16, 32, 64}) }; // word size in bits (8, 16, 32, 64)
  const Direction id { input.get_choice("-id", 1, {1, 2}) }; // default is 1 (lsb to msb)
  const bool iw = input.exists("-iw"); // byte swap for binary input (endianness change)
  const uint64_t count { input.get_uint64("-c", 0) }; // maximum number of windows counted, 0 = until EOF
  const uint64_t n { input.get_uint64("-n", 1) }; // size of sliding window
  const uint64_t maxlen { input.get_uint64("-x", 32) }; // maxlen for counting zeros and ones
  verbose = input.exists("-v");
  debug = input.exists("-d");
  const bool print_table { input.exists("-t") }; // table of probabilities and counts

  const uint64_t bins = histogram_bins(n);
  const uint64_t mask = bins-1;
  const auto run_size = run_array_size(maxlen);

  if (verbose) {
    msg << "bitent " << GIT_HASH << " " << __DATE__ << " " << __TIME__ << std::endl;
    msg << "filename=" << (filename != "" ? filename : "[stdin]") << std::endl;
    msg << "input word size ws=" << ws << std::endl;
    msg << "input bit direction id=" << static_cast<int>(id) << " [" << name(id) << "]" << std::endl;
    msg << "input endianness swap iw=" << std::boolalpha << iw << std::endl;
    show_with_logs(msg, "count", count, true);
    msg << "window size n=" << n << " [" << bins << " bins, mask=" << std::bitset<16>(mask) << "]" << std::endl;
    msg << "maxlen=" << maxlen << std::endl;
  }

  auto gen = boolreader(filename, ws, id, iw);
  vc acc(bins, 0); // accumulator
  uint64_t symbol { 0 };
  uint64_t nrread { 0 };
  uint64_t ones { 0 };
  uint64_t zeros { 0 };
  bool prev { false };
  std::vector<uint64_t> cntones(run_size, 0);
  std::vector<uint64_t> cntzeros(run_size, 0);
  std::vector<uint64_t> cntother(run_size, 0);
  for (uint64_t i = 0; i < count || count == 0;) { // i not updated here!
   try {
     bool b = (*gen)();
     auto old = symbol;
     symbol = mask & ((symbol<<1) + (b ? 1 : 0));
     nrread++;
     if (nrread >= n) {
       acc[symbol]++;
       i++;
     }
     if (b) {
       if (prev) {
         ones++;
       } else {
         ones=1;
         zeros=0;
       }
     }
     if (!b) {
       if (!prev) {
         zeros++;
       } else {
         zeros=1;
         ones=0;
       }
     }
     prev = b;
     for (uint64_t j = 1; j <= maxlen; j++) {
       if (nrread >= j) {
         if (ones >= j)
           cntones[j]++;
         else if (zeros >= j)
           cntzeros[j]++;
         else
           cntother[j]++;
       }
     }
     if (debug) std::cout << "old=" << std::bitset<16>(old) << " b=" << (b?1:0) << " symbol=" << std::bitset<16>(symbol) << " nrread=" << nrread << " ii=" << i << std::endl;
   } catch (const EOF_exception &) {
     break;
   } catch (const std::exception& e) {
     std::cerr << "Error: " << e.what() << std::endl;
     exit(1);
   }
  }

  uint64_t nr = std::accumulate(acc.begin(), acc.end(), uint64_t(0));
  if (nr == 0) {
    std::cerr << "Error: no complete " << n << "-bit window was read (" << nrread << " bits read)." << std::endl;
    return 1;
  }
  show_with_logs(std::cout, "nr", nr);

  std::vector<double> P(bins); // probabilities
  for (uint64_t i = 0; i < bins; i++)
    P[i] = double(acc[i]) / nr;

  if (print_table) {
    for (uint64_t i = 0; i < bins; i++) {
      std::cout << "P[" << bits(i,n) << " ~ " << std::setw(4) << std::right << i << "]=" << std::setw(20) << std::setprecision(14) << std::left << P[i] << "  " << acc[i] << std::endl;
    }
  }

  uint64_t origin = bins; // first occupied bin; nr > 0 guarantees one exists
  double mean_shifted = 0.0;
  double Pmin = std::numeric_limits<double>::max();
  double Pmax = std::numeric_limits<double>::min();
  for (uint64_t i = 0; i < bins; i++) {
    if (P[i] > 0.0) {
      if (origin == bins) origin = i;
      mean_shifted += double(i-origin)*P[i];
    }
    Pmin = std::min(Pmin, P[i]);
    Pmax = std::max(Pmax, P[i]);
  }
  const double mean = double(origin)+mean_shifted;
  // Center in shifted coordinates to avoid cancellation at large symbol indices.
  double var = 0.0;
  for (uint64_t i = origin; i < bins; i++) {
    const double delta = double(i-origin)-mean_shifted;
    var += delta*delta*P[i];
  }
  const double stddev = sqrt(var);
  const double Hmin1 = -log2(Pmax);
  std::cout << "mean=" << mean << std::endl;
  std::cout << "var=" << var << std::endl;
  std::cout << "stddev=" << stddev << std::endl;
  std::cout << "Pmin=" << Pmin << std::endl;
  std::cout << "Pmax=" << Pmax << " Hmin=" << Hmin1 << std::endl;

  double HA = 0.0;
  double HminA = std::numeric_limits<double>::max();
  for (uint64_t i = 0; i < bins; i++) {
    if (P[i] == 0.0) continue; // use the convention 0*log2(0)=0
    HA += -log2(P[i])*P[i];
    HminA = std::min(HminA, -log2(P[i]));
  }
  const double HAperbit = HA/n;
  const double HminAperbit = HminA/n;
  std::cout << "H=" << HA << " = " << HAperbit << " per bit"
    << "   Hmin=" << HminA << " = " << HminAperbit << " per bit" << std::endl;

  std::vector<uint64_t> cnttotal(run_size, 0);
  std::vector<double> cntP0(run_size, 0);
  std::vector<double> cntP1(run_size, 0);
  std::vector<double> cntHmin(run_size, 0);
  std::vector<double> cntHminperbit(run_size, 0);
  double Hminpbmin = std::numeric_limits<double>::max();
  const uint64_t atleast = 1000;
  for (uint64_t j = 1; j <= maxlen; j++) {
    cnttotal[j] = cntones[j]+cntzeros[j]+cntother[j];
    if (cnttotal[j] == 0) {
      cntP0[j] = cntP1[j] = cntHmin[j] = cntHminperbit[j] = std::numeric_limits<double>::quiet_NaN();
    } else {
      cntP0[j] = double(cntzeros[j])/cnttotal[j];
      cntP1[j] = double(cntones[j])/cnttotal[j];
      const auto pmax = std::max(cntP0[j], cntP1[j]);
      cntHmin[j] = pmax > 0 ? -log2(pmax) : std::numeric_limits<double>::infinity();
      cntHminperbit[j] = cntHmin[j]/j;
    }
    std::cout << "len=" << std::setw(3) << j << " "
      << " 0:nr=" << cntzeros[j] << ",p=" << cntP0[j]
      << " 1:nr=" << cntones[j]  << ",p=" << cntP1[j]
      << " Hmin= " << cntHmin[j]
      << " = " << cntHminperbit[j] << " per bit " << std::endl;
    if (cntzeros[j] >= atleast || cntones[j] >= atleast) Hminpbmin = std::min(Hminpbmin, cntHminperbit[j]);
  }

  const double Hminperbit = std::min(HminAperbit, Hminpbmin);
  std::cout << "Hmin = " << Hminperbit << " per bit" << std::endl;
  return 0;
} catch (const std::bad_alloc &) {
  std::cerr << "Error: memory allocation failed; reduce -n or -x." << std::endl;
  return 1;
} catch (const std::exception &e) {
  std::cerr << "Error: " << e.what() << std::endl;
  return 1;
}
