#include <array>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../filter.h"

static_assert(std::is_same_v<decltype(filter(0.0, IntFilter::round)), int>);

void require(bool condition, const std::string &message)
{
  if (!condition) throw std::runtime_error(message);
}

std::string label(double x, IntFilter mode)
{
  std::ostringstream text;
  text << name(mode) << '(' << std::setprecision(17) << x << ')';
  return text.str();
}

void equal(double x, IntFilter mode, int expected)
{
  require(filter(x, mode) == expected, label(x, mode) + " has the wrong result");
}

template <typename E>
void rejected(double x, IntFilter mode)
{
  try {
    filter(x, mode);
  } catch (const E &) {
    return;
  }
  throw std::runtime_error(label(x, mode) + " was not rejected");
}

int main(int argc, char *argv[])
{
  try {
    const std::array modes {IntFilter::round, IntFilter::floor, IntFilter::ceil, IntFilter::trunc};
    struct Example { double x; std::array<int, 4> expected; };
    for (const auto &example : {
          Example{-2.5, {-3, -3, -2, -2}}, {-1.9, {-2, -2, -1, -1}},
          {-1.5, {-2, -2, -1, -1}}, {-1.1, {-1, -2, -1, -1}},
          {-0.9, {-1, -1, 0, 0}}, {-0.5, {-1, -1, 0, 0}},
          {-0.1, {0, -1, 0, 0}}, {-0.0, {0, 0, 0, 0}},
          {0.0, {0, 0, 0, 0}}, {0.1, {0, 0, 1, 0}},
          {0.5, {1, 0, 1, 0}}, {0.9, {1, 0, 1, 0}},
          {1.1, {1, 1, 2, 1}}, {1.5, {2, 1, 2, 1}},
          {1.9, {2, 1, 2, 1}}, {2.5, {3, 2, 3, 2}}}) {
      for (std::size_t i = 0; i < modes.size(); i++)
        equal(example.x, modes[i], example.expected[i]);
    }
    const auto tiny = std::numeric_limits<double>::denorm_min();
    equal(tiny, IntFilter::ceil, 1);
    equal(-tiny, IntFilter::floor, -1);
    for (const auto mode : {IntFilter::round, IntFilter::trunc}) {
      equal(tiny, mode, 0);
      equal(-tiny, mode, 0);
    }
    equal(std::nextafter(0.5, 0.0), IntFilter::round, 0);
    equal(std::nextafter(-0.5, 0.0), IntFilter::round, 0);

    const int low = std::numeric_limits<int>::min(), high = std::numeric_limits<int>::max();
    const double lo = low, hi = high;
    const auto inf = std::numeric_limits<double>::infinity();
    for (const auto mode : modes) {
      equal(lo, mode, low);
      equal(hi, mode, high);
    }
    equal(std::nextafter(hi+0.5, -inf), IntFilter::round, high);
    equal(std::nextafter(lo-0.5, inf), IntFilter::round, low);
    equal(hi+0.75, IntFilter::floor, high);
    equal(hi+0.75, IntFilter::trunc, high);
    equal(lo-0.75, IntFilter::ceil, low);
    equal(lo-0.75, IntFilter::trunc, low);
    equal(std::nextafter(hi+1.0, -inf), IntFilter::floor, high);
    equal(std::nextafter(lo-1.0, inf), IntFilter::ceil, low);
    std::cout << "PASS: rounding rules and representable boundary results" << std::endl;

    if (argc == 2 && std::string(argv[1]) == "--valid-only") return 0;
    require(argc == 1, "unknown test argument");
    for (const auto mode : modes) {
      for (const auto x : {hi+1.0, lo-1.0, std::numeric_limits<double>::max(),
                           -std::numeric_limits<double>::max()})
        rejected<std::out_of_range>(x, mode);
      for (const auto x : {inf, -inf, std::numeric_limits<double>::quiet_NaN()})
        rejected<std::domain_error>(x, mode);
    }
    rejected<std::out_of_range>(hi+0.5, IntFilter::round);
    rejected<std::out_of_range>(lo-0.5, IntFilter::round);
    rejected<std::out_of_range>(std::nextafter(hi, inf), IntFilter::ceil);
    rejected<std::out_of_range>(std::nextafter(lo, -inf), IntFilter::floor);
    for (const int mode : {-1, 4, 99})
      rejected<std::invalid_argument>(0.0, static_cast<IntFilter>(mode));
    std::cout << "PASS: range errors, non-finite values, and invalid filter codes" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
