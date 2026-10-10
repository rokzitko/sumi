// Exercise the health-test decision directly, including invalid statistics
// that cannot be produced by valid finite integer samples through the CLI.
#define main sumi_main
#include "../sumi.cc"
#undef main

#include <sstream>

void require(bool condition, const std::string &message)
{
  if (!condition) throw std::runtime_error(message);
}

void require_rejected(double rvar)
{
  try {
    health_test_passes(rvar, 0.5);
  } catch (const std::runtime_error &) {
    return;
  }
  throw std::runtime_error("Non-finite health-test variance was classified.");
}

int main()
{
  try {
    require(health_test_passes(0.0, 0.0), "zero threshold equality");
    require(!health_test_passes(0.0, 0.1), "constant block at positive threshold");
    require(health_test_passes(0.5, 0.5), "positive threshold equality");
    require(!health_test_passes(0.49, 0.5), "variance below threshold");
    require(health_test_passes(0.51, 0.5), "variance above threshold");
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    for (const auto rvar : {nan, inf, -inf}) require_rejected(rvar);

    for (const auto &values : {std::vector<double>{}, {1.0}, {0.0, nan}, {0.0, inf}, {0.0, -inf}}) {
      std::ostringstream sink;
      Stats<> stats(sink, "sample");
      for (const auto x : values) stats.add(x);
      stats.stats();
      require_rejected(stats.get_rvar());
    }
    std::cout << "PASS: health-test decisions and non-finite statistics" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
