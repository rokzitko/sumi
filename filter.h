#ifndef _filter_h_
#define _filter_h_

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

enum class IntFilter {
  round = 0,
  floor = 1,
  ceil = 2,
  trunc = 3
};

std::string name(IntFilter i) 
{
  using enum IntFilter;
  switch (i) {
  case round:
    return "round";
  case floor:
    return "floor";
  case ceil:
    return "ceil";
  case trunc:
    return "trunc";
  default:
    return "undefined";
  }
}

auto filter(double x, IntFilter flt)
{
  if (!std::isfinite(x))
    throw std::domain_error("Cannot convert a non-finite sample to integer.");
  double n;
  switch (flt) {
  case IntFilter::round:
    n = std::round(x);
    break;
  case IntFilter::floor:
    n = std::floor(x);
    break;
  case IntFilter::ceil:
    n = std::ceil(x);
    break;
  case IntFilter::trunc:
    n = std::trunc(x);
    break;
  default:
    throw std::invalid_argument("Unknown integer filter; use -f 0, 1, 2, or 3.");
  }
  // Check the filtered result. The exclusive upper bound is exactly representable.
  const double lower = std::numeric_limits<int>::min();
  const double upper = -lower;
  if (n < lower || n >= upper)
    throw std::out_of_range("Filtered sample is outside the integer range.");
  return static_cast<int>(n);
}

#endif
