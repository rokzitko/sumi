#ifndef _stats_h_
#define _stats_h_

#include <ostream>
#include <string>
#include <cstdint>
#include <cmath>

#include "common.h"

template <typename D = double>
  class Stats {
  private:
    // Accumulators
    uint64_t cnt = 0;
    D sumx = 0;    // sum_i x_i
    D sumx2 = 0;   // sum_i x_i^2
    D sumdx2 = 0;  // sum_i (x_i-x_{i-1})^2
    D sumdy2 = 0;  // sum_i (x_i-2x_{i-1}+x_{i-2})^2
    D suma1 = 0;   // sum_i x_i x_{i-1}
    D suma2 = 0;   // sum_i x_i x_{i-2}
    D suma3 = 0;   // sum_i x_i x_{i-3}
    // Temporary variables
    D xprev1 = 0;
    D xprev2 = 0;
    D xprev3 = 0;
    // Estimates
    D mean, var, stddev;
    D rvar, rdev;
    D avar, adev;
    D tvar, tdev;
    D R0, R1, R2, R3;
    // Output control
    int w = 12; // width
    int p = 6; // precision
    bool report_on_exit;
    std::ostream &_F;
    std::string _prefix;
  public:
    Stats(std::ostream &F, std::string prefix) :
      _F(F), _prefix(prefix)
    {
      report_on_exit = true;
    }
    Stats() {
      report_on_exit = false;
    }
    void add(const D x) {
      cnt++;
      sumx += x;
      sumx2 += x*x;
      if (cnt >= 2) {
        auto q = x-xprev1;
        sumdx2 += q*q;
      }
      if (cnt >= 3) {
        auto q = x-2*xprev1+xprev2;
        sumdy2 += q*q;
      }
      if (cnt >= 2)
        suma1 += x*xprev1;
      if (cnt >= 3)
        suma2 += x*xprev2;
      if (cnt >= 4)
        suma3 += x*xprev3;
      xprev3 = xprev2;
      xprev2 = xprev1;
      xprev1 = x;
    }
    void stats() {
      mean = sumx/cnt;
      var = sumx2/cnt-mean*mean;
      stddev = sqrt(var);
      rvar = 0.5*sumdx2/(cnt-1); // "Rok's variance"
      rdev = sqrt(rvar);
      // https://en.wikipedia.org/wiki/Allan_variance
      avar = 0.5*sumdy2/(cnt-2); // Allen variance
      adev = sqrt(avar);
      // https://en.wikipedia.org/wiki/Time_deviation
      tvar = avar/3.0; // "time deviation" (well, not quite!)
      tdev = sqrt(tvar);
      // serial correlation coefficients
      // https://en.wikipedia.org/wiki/Autocorrelation
      R0 = (sumx2/cnt-mean*mean)/var;
      R1 = (suma1/(cnt-1)-mean*mean)/var;
      R2 = (suma2/(cnt-2)-mean*mean)/var;
      R3 = (suma3/(cnt-3)-mean*mean)/var;
    }
    void report(std::ostream &F, std::string prefix) {
      F << prefix << " "; show_with_logs(F, "cnt", cnt);
      F << std::setprecision(p);
      F << prefix << " mean=" << mean << std::endl;
      F << prefix << "  var=" << std::setw(w) << var << "  ";
      F << " stddev=" << stddev << std::endl;
      F << prefix << " rvar=" << std::setw(w) << rvar << "  ";
      F << "   rdev=" << rdev << std::endl;
      F << prefix << " tvar=" << std::setw(w) << tvar << "  ";
      F << "   tdev=" << tdev << std::endl;
      F << prefix << " avar=" << std::setw(w) << avar << "  ";
      F << "   adev=" << adev << std::endl;
      F << prefix << " serial corr. R=" << R0 << " " << R1 << " " << R2 << " " << R3 << std::endl;
    }
    ~Stats() {
      if (report_on_exit && cnt >= 4) {
        stats();
        report(_F, _prefix);
      }
    }
  };

#endif
