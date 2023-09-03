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
    D sumx = 0;
    D sumx2 = 0;
    D sumdx2 = 0;
    D sumdy2 = 0;
    D xprev1 = 0;
    D xprev2 = 0;
    // Estimates
    D mean, var, stddev;
    D rvar, rdev;
    D avar, adev;
    D tvar, tdev;
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
      xprev2 = xprev1;
      xprev1 = x;
    }
    void stats() {
      mean = sumx/cnt;
      var = sumx2/cnt-mean*mean;
      stddev = sqrt(var);
      rvar = 0.5*sumdx2/(cnt-1); // Rok's variance
      rdev = sqrt(rvar);
      // https://en.wikipedia.org/wiki/Allan_variance
      avar = 0.5*sumdy2/(cnt-2); // Allen variance
      adev = sqrt(avar);
      // https://en.wikipedia.org/wiki/Time_deviation
      tvar = avar/3.0; // "time deviation" (well, not quite!)
      tdev = sqrt(tvar);
    }
    void report(std::ostream &F, std::string prefix) {
      F << prefix << " "; show_with_logs(F, "cnt", cnt);
      F << prefix << " mean=" << mean << std::endl;
      F << prefix << " var=" << var << std::endl;
      F << prefix << " stddev=" << stddev << std::endl;
      F << prefix << " rvar=" << rvar << std::endl;
      F << prefix << " rdev=" << rdev << std::endl;
      F << prefix << " tvar=" << tvar << std::endl;
      F << prefix << " tdev=" << tdev << std::endl;
      F << prefix << " avar=" << avar << std::endl;
      F << prefix << " adev=" << adev << std::endl;
    }
    ~Stats() {
      if (report_on_exit && cnt >= 2) {
        stats();
        report(_F, _prefix);
      }
    }
  };

#endif
