// Calculate Shannon and minimal entropy for a bit series.
// Rok Zitko, 2023

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <random>
#include <bitset>
#include <stdexcept>
#include <ctime>
#include <cassert>
#include <coroutine>
#include <math.h>
#include <limits>
#include <unistd.h>

#include <complex>

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

int main(int argc, char *argv[])
{
  InputParser input(argc, argv);

  const std::string filename { input.exists("-i") ? input.get("-i") : "" };
  const int ws { input.exists("-ws") ? std::stoi(input.get("-ws")) : 8 }; // word size in bits (8, 16, 32, 64)
  const Direction id { input.exists("-id") ? std::stoi(input.get("-id")) : 1 }; // default is 1 (lsb to msb)
  const bool iw = input.exists("-iw"); // byte swap for binary input (endianness change)
  const uint64_t count { input.exists("-c") ? std::stoul(input.get("-c")) : 0 }; // number of random numbers read, 0 = infinite (until EOF)
  const int n { input.exists("-n") ? std::stoi(input.get("-n")) : 1 }; // size of sliding window
  verbose = input.exists("-v");
  debug = input.exists("-d");
  const bool print_table { input.exists("-t") }; // table of probabilities and counts

  const uint64_t bins = 1<<n;
  const uint64_t mask = bins-1;

  if (verbose) {
    msg << "filename=" << (filename != "" ? filename : "[stdin]") << std::endl;
    msg << "input word size ws=" << ws << std::endl;
    msg << "input bit direction id=" << static_cast<int>(id) << " [" << name(id) << "]" << std::endl;
    msg << "input endianness swap iw=" << std::boolalpha << iw << std::endl;
    showcount(msg, count);
    msg << "window size n=" << n << " [" << bins << " bins, mask=" << std::bitset<16>(mask) << "]" << std::endl;
  }

  auto gen = boolreader(filename, ws, id, iw);
  vc acc(bins, 0); // accumulator
  uint64_t symbol { 0 };
  int nrread { 0 };
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
     if (debug) std::cout << "old=" << std::bitset<16>(old) << " b=" << (b?1:0) << " symbol=" << std::bitset<16>(symbol) << " nrread=" << nrread << " ii=" << i << std::endl;
   } catch (const EOF_exception &) {
     break;
   } catch (const std::exception& e) {
     std::cerr << "Error: " << e.what() << std::endl;
     exit(1);
   }
  }

  uint64_t nr = std::accumulate(acc.begin(), acc.end(), 0);
  std::cout << "nr=" << nr << std::endl;

  std::vector<double> P(bins); // probabilities
  for (uint64_t i = 0; i < bins; i++)
    P[i] = double(acc[i]) / nr;

  if (print_table) {
    for (uint64_t i = 0; i < bins; i++) {
      std::cout << "P[" << bits(i,n) << " ~ " << std::setw(4) << std::right << i << "]=" << std::setw(20) << std::setprecision(14) << std::left << P[i] << "  " << acc[i] << std::endl;
    }
  }

  double mean = 0.0;
  double sq = 0.0;
  double Pmin = std::numeric_limits<double>::max();
  double Pmax = std::numeric_limits<double>::min();
  for (uint64_t i = 0; i < bins; i++) {
    mean += double(i)*P[i];
    sq += double(i)*double(i)*P[i];
    Pmin = std::min(Pmin, P[i]);
    Pmax = std::max(Pmax, P[i]);
  }
  const double var = sq-mean*mean;
  const double stddev = sqrt(var);
  const double Hmin1 = -log2(Pmax);
  std::cout << "mean=" << mean << std::endl;
  std::cout << "var=" << var << std::endl;
  std::cout << "stddev=" << stddev << std::endl;
  std::cout << "Pmin=" << Pmin << std::endl;
  std::cout << "Pmax=" << Pmax << " Hmin=" << Hmin1 << std::endl;

  double H = 0.0;
  double Hmin = std::numeric_limits<double>::max();
  for (uint64_t i = 0; i < bins; i++) {
    H += -log2(P[i])*P[i];
    Hmin = std::min(Hmin, -log2(P[i]));
  }
  const double Hperbit = H/n;
  const double Hminperbit = Hmin/n;
  std::cout << "H=" << H << " = " << Hperbit << " per bit" << std::endl;
  std::cout << "Hmin=" << Hmin << " = " << Hminperbit << " per bit" << std::endl;
}
