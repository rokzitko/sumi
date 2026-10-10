#ifndef _stats_h_
#define _stats_h_

#include <ostream>
#include <string>
#include <cstdint>
#include <cmath>
#include <limits>

#include "common.h"

template <typename D = double>
  class Stats {
  private:
    struct LagMoments {
      D mean_late = 0;
      D mean_early = 0;
      D cross = 0; // sum of products centered on the two pair means
      void add(const D late, const D early, uint64_t n) {
        const auto delta_late = late-mean_late;
        const auto delta_early = early-mean_early;
        mean_late += delta_late/n;
        mean_early += delta_early/n;
        cross += delta_late*(early-mean_early);
      }
      D covariance(const D center, uint64_t n) const {
        // Convert from the pair means to the full-series mean.
        return cross/n+(mean_late-center)*(mean_early-center);
      }
    };
    // Accumulators
    uint64_t cnt = 0;
    D meanx = 0;   // running mean
    D m2 = 0;      // sum_i (x_i-meanx)^2
    D sumdx2 = 0;  // sum_i (x_i-x_{i-1})^2
    D sumdy2 = 0;  // sum_i (x_i-2x_{i-1}+x_{i-2})^2
    LagMoments lag1, lag2, lag3;
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
    void reset() { // reset accumulators
      cnt = 0;
      meanx = m2 = sumdx2 = sumdy2 = 0;
      lag1 = lag2 = lag3 = {};
    }
    void add(const D x) {
      cnt++;
      // Welford update avoids subtracting large, nearly equal second moments.
      const auto delta = x-meanx;
      meanx += delta/cnt;
      m2 += delta*(x-meanx);
      if (cnt >= 2) {
        auto q = x-xprev1;
        sumdx2 += q*q;
      }
      if (cnt >= 3) {
        auto q = x-2*xprev1+xprev2;
        sumdy2 += q*q;
      }
      if (cnt >= 2)
        lag1.add(x, xprev1, cnt-1);
      if (cnt >= 3)
        lag2.add(x, xprev2, cnt-2);
      if (cnt >= 4)
        lag3.add(x, xprev3, cnt-3);
      xprev3 = xprev2;
      xprev2 = xprev1;
      xprev1 = x;
    }
    void stats() {
      mean = cnt ? meanx : std::numeric_limits<D>::quiet_NaN();
      var = m2/cnt; // population variance
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
      R0 = var/var; // unity for finite positive variance, undefined for a constant series
      // Full-series centering with cnt-k pair-count normalization at lag k.
      const auto nan = std::numeric_limits<D>::quiet_NaN();
      R1 = cnt > 1 && var > 0 ? lag1.covariance(mean, cnt-1)/var : nan;
      R2 = cnt > 2 && var > 0 ? lag2.covariance(mean, cnt-2)/var : nan;
      R3 = cnt > 3 && var > 0 ? lag3.covariance(mean, cnt-3)/var : nan;
    }
    D get_rvar() {
       return rvar;
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
