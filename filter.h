#ifndef _filter_h_
#define _filter_h_

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
  int n;
  switch (flt) {
  case IntFilter::round:
    n = round(x);
    break;
  case IntFilter::floor:
    n = floor(x);
    break;
  case IntFilter::ceil:
    n = ceil(x);
    break;
  case IntFilter::trunc:
    n = trunc(x);
    break;
  default:
    assert(false);
  }
  return n;
}

#endif
