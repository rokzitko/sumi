#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "../stats.h"

void require(bool condition, const std::string &message)
{
  if (!condition) throw std::runtime_error(message);
}

void check_close(double actual, double expected, const std::string &label)
{
  // report() prints six significant digits. Zero is required to remain exact.
  require(std::isfinite(actual) && std::abs(actual-expected) <= 5e-6*std::abs(expected),
          label + ": got " + std::to_string(actual) + ", expected " + std::to_string(expected));
}

void check(Stats<> &stats, const std::vector<double> &values, const std::string &label)
{
  stats.stats();
  std::ostringstream output;
  stats.report(output, "sample");
  const auto text = output.str();
  auto value = [&text](const std::string &key) {
    const auto pos = text.find(key);
    require(pos != std::string::npos, "missing report field " + key);
    return std::stod(text.substr(pos+key.size()));
  };
  if (values.size() < 2)
    for (const auto key : {"rvar=", "rdev="})
      require(std::isnan(value(key)), label + " undefined field " + key);
  if (values.size() < 3)
    for (const auto key : {"avar=", "adev=", "tvar=", "tdev="})
      require(std::isnan(value(key)), label + " undefined field " + key);
  if (values.empty()) {
    for (const auto key : {"mean=", " var=", "stddev=", "R="})
      require(std::isnan(value(key)), label + " undefined field " + key);
    std::cout << "PASS: " << label << std::endl;
    return;
  }

  // Independent two-pass reference, centered on the first sample so the
  // reference itself does not subtract large, nearly equal second moments.
  const long double origin = values.front();
  long double center = 0;
  for (const auto x : values) center += x-origin;
  center /= values.size();
  long double variance = 0;
  for (const auto x : values) {
    const auto delta = (x-origin)-center;
    variance += delta*delta;
  }
  variance /= values.size(); // Population variance, not the N-1 estimator.
  check_close(value("mean="), origin+center, label + " mean");
  check_close(value(" var="), variance, label + " variance");
  check_close(value("stddev="), std::sqrt(variance), label + " standard deviation");
  if (variance > 0)
    require(value("R=") == 1.0, label + " zero-lag correlation");
  else
    require(std::isnan(value("R=")), label + " constant-series correlation must be undefined");

  // Difference-based variances use separate accumulators, including after reset.
  if (values.size() >= 3) {
    double dx2 = 0, dy2 = 0;
    for (std::size_t i = 1; i < values.size(); i++) {
      const auto dx = values[i]-values[i-1];
      dx2 += dx*dx;
      if (i >= 2) {
        const auto dy = values[i]-2*values[i-1]+values[i-2];
        dy2 += dy*dy;
      }
    }
    check_close(value("rvar="), 0.5*dx2/(values.size()-1), label + " first differences");
    check_close(value("avar="), 0.5*dy2/(values.size()-2), label + " second differences");
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
    check({}, "empty sample");
    check({3.0}, "one sample");
    check({1.0, 3.0}, "two samples, population normalization");
    check({1.0, 2.0, 3.0, 4.0}, "known mean 2.5 and variance 1.25");
    check(std::vector<double>(32, 0.0), "constant zero");
    check(std::vector<double>(32, -7.0), "constant nonzero");
    check({1e12+1, 1e12+2, 1e12+3, 1e12+4}, "known variance at large offset");
    check(std::vector<double>(1000, 1e12), "large constant");

    std::default_random_engine dre(1234);
    std::normal_distribution<double> normal(0.0, 1.0);
    std::vector<double> samples(100000);
    for (auto &x : samples) x = normal(dre);
    check(samples, "seeded normal samples");
    for (const double offset : {1e8, -1e8}) {
      auto shifted = samples;
      for (auto &x : shifted) x += offset;
      check(shifted, "seeded normal samples, offset=" + std::to_string(offset));
      std::reverse(shifted.begin(), shifted.end());
      check(shifted, "reversed samples, offset=" + std::to_string(offset));
    }

    std::ostringstream sink;
    Stats<> stats(sink, "sample");
    for (const auto x : samples) stats.add(x+1e8);
    stats.stats();
    stats.reset();
    check(stats, {}, "empty sample after reset");
    std::vector<double> values {1, 2, 4, 8};
    for (const auto x : values) stats.add(x);
    check(stats, values, "reset after a large-offset sequence");
    check(stats, values, "repeated reporting");
    for (const auto x : {16.0, 32.0}) {
      stats.add(x);
      values.push_back(x);
    }
    check(stats, values, "continued accumulation after reporting");
    stats.reset();
    values.assign(8, 0.0);
    for (const auto x : values) stats.add(x);
    check(stats, values, "reset to a constant sequence");
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
