#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "../stats.h"

void require(bool condition, const std::string &message)
{
  if (!condition) throw std::runtime_error(message);
}

std::array<double, 4> reference(const std::vector<double> &values)
{
  std::array<double, 4> result;
  result.fill(std::numeric_limits<double>::quiet_NaN());
  if (values.empty()) return result;
  // Two-pass reference in shifted coordinates, retaining the full record.
  const long double origin = values.front();
  long double mean = 0, variance = 0;
  for (const auto x : values) mean += x-origin;
  mean /= values.size();
  std::vector<long double> centered;
  for (const auto x : values) {
    const auto delta = (x-origin)-mean;
    centered.push_back(delta);
    variance += delta*delta;
  }
  variance /= values.size();
  if (variance == 0) return result;
  result[0] = 1;
  for (std::size_t lag = 1; lag < result.size() && lag < values.size(); lag++) {
    long double cross = 0;
    for (std::size_t i = lag; i < values.size(); i++)
      cross += centered[i]*centered[i-lag];
    result[lag] = cross/(values.size()-lag)/variance;
  }
  return result;
}

void check(Stats<> &stats, const std::vector<double> &values, const std::string &label)
{
  stats.stats();
  std::ostringstream output;
  stats.report(output, "sample");
  const auto text = output.str();
  const auto pos = text.find("R=");
  require(pos != std::string::npos, "missing correlations");
  std::istringstream fields(text.substr(pos+2));
  const auto expected = reference(values);
  for (std::size_t lag = 0; lag < expected.size(); lag++) {
    std::string token;
    require(bool(fields >> token), "missing correlation field");
    const auto actual = std::stod(token);
    const auto message = label + " R" + std::to_string(lag) + ": got " + token
                         + ", expected " + std::to_string(expected[lag]);
    if (std::isnan(expected[lag])) {
      require(std::isnan(actual), message);
    } else {
      // Six-significant-digit report rounding, plus roundoff at zero.
      const auto tolerance = 5e-6*std::abs(expected[lag])+1e-12;
      require(std::isfinite(actual) && std::abs(actual-expected[lag]) <= tolerance, message);
    }
  }
  std::cout << "PASS: " << label << std::endl;
}

void check(const std::vector<double> &values, const std::string &label)
{
  std::ostringstream sink;
  Stats<> stats(sink, "sample");
  for (const auto x : values) stats.add(x);
  check(stats, values, label);
}

int main()
{
  try {
    // The chosen N-lag normalization gives R1=1/3, R2=-3/5, R3=-9/5.
    check({-1.5, -0.5, 0.5, 1.5}, "zero-mean linear sequence, N-lag normalization");
    check({-2.75, -1.75, 0.25, 4.25}, "zero-mean asymmetric sequence");
    check({1, 2, 4, 8}, "shifted asymmetric sequence");
    check({1, 2, 3, 4}, "linear sequence, N-lag normalization");
    check({1e12+1, 1e12+2, 1e12+3, 1e12+4}, "linear sequence at large offset");
    check({}, "empty sequence");
    check({3}, "one sample");
    check({1, 3}, "two samples, missing higher lags");
    check({1, 3, 2}, "three samples, zero correlation and missing lag three");
    check(std::vector<double>(16, 0), "constant zero");
    check(std::vector<double>(1000, 1e12), "large constant");
    check({-1, 1, -1, 1, -1, 1, -1, 1}, "alternating sequence");

    std::default_random_engine dre(1234);
    std::normal_distribution<double> normal(0.0, 1.0);
    std::vector<double> white(4096), correlated(4096);
    double state = 0;
    for (std::size_t i = 0; i < white.size(); i++) {
      white[i] = normal(dre);
      state = 0.8*state+white[i];
      correlated[i] = state;
    }
    for (const bool use_correlated : {false, true}) {
      const auto &values = use_correlated ? correlated : white;
      const std::string kind = use_correlated ? "correlated" : "white";
      for (const double offset : {0.0, 10.0, 1e6, 1e8, -1e8}) {
        auto transformed = values;
        for (auto &x : transformed) x += offset;
        const auto label = kind + " sequence, offset=" + std::to_string(offset);
        check(transformed, label);
        std::reverse(transformed.begin(), transformed.end());
        check(transformed, "reversed " + label);
      }
      auto scaled = values;
      for (auto &x : scaled) x *= -2.5;
      check(scaled, kind + " negative scaling");
    }

    std::ostringstream sink;
    Stats<> stats(sink, "sample");
    for (const auto x : correlated) stats.add(x+1e8);
    stats.stats();
    stats.reset();
    check(stats, {}, "empty after reset");
    std::vector<double> values;
    for (const auto x : {1.0, 2.0, 4.0, 8.0, 16.0, 32.0}) {
      stats.add(x);
      values.push_back(x);
      check(stats, values, "continued accumulation, count=" + std::to_string(values.size()));
    }
    check(stats, values, "repeated reporting");
    stats.reset();
    values.assign(8, 7.0);
    for (const auto x : values) stats.add(x);
    check(stats, values, "reset to a constant sequence");
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
