#include <cstdlib>
#include <new>
#include <sstream>

// A single injected allocation failure tests the CLI's error handling without
// trying to exhaust real memory. This replacement exists only in this test.
bool fail_next_allocation = false;

void *operator new(std::size_t size)
{
  if (fail_next_allocation) {
    fail_next_allocation = false;
    throw std::bad_alloc();
  }
  if (auto p = std::malloc(size ? size : 1)) return p;
  throw std::bad_alloc();
}

void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }

#define main bitent_main
#include "../bitent.cc"
#undef main

void require(bool condition, const std::string &message)
{
  if (!condition) throw std::runtime_error(message);
}

template <typename F>
void require_rejected(F f)
{
  try {
    f();
  } catch (const std::invalid_argument &) {
    return;
  } catch (const std::length_error &) {
    return;
  }
  throw std::runtime_error("Invalid size was accepted.");
}

int main()
{
  try {
    require(histogram_bins(1) == 2, "minimum window size");
    require(histogram_bins(20) == 1048576, "intermediate window size");
    require(histogram_bins(30) == 1073741824, "maximum window size");
    for (const uint64_t n : {uint64_t{0}, uint64_t{31}, uint64_t{32}, uint64_t{63},
                            uint64_t{64}, std::numeric_limits<uint64_t>::max()})
      require_rejected([n] { histogram_bins(n); });
    std::cout << "PASS: histogram limits and unsigned sizing" << std::endl;

    const auto capacity = std::min(vc{}.max_size(), std::vector<double>{}.max_size());
    require(run_array_size(0) == 1, "disabled run report");
    require(run_array_size(32) == 33, "default run length");
    require(run_array_size(uint64_t{1} << 32) == (uint64_t{1} << 32)+1, "wide run index");
    require(run_array_size(capacity-1) == capacity, "last representable run-array size");
    require_rejected([&] { run_array_size(capacity); });
    require_rejected([] { run_array_size(std::numeric_limits<uint64_t>::max()); });
    std::cout << "PASS: run-array limits without large allocations" << std::endl;

    char program[] = "bitent", option[] = "-n", argument[] = "1";
    char *argv[] = {program, option, argument, nullptr};
    std::ostringstream errors;
    auto saved = std::cerr.rdbuf(errors.rdbuf());
    fail_next_allocation = true;
    int status;
    try {
      status = bitent_main(3, argv);
    } catch (...) {
      fail_next_allocation = false;
      std::cerr.rdbuf(saved);
      throw;
    }
    const bool injected = !fail_next_allocation;
    fail_next_allocation = false;
    std::cerr.rdbuf(saved);
    require(injected && status == 1, "allocation failure did not return an error");
    require(errors.str().find("memory allocation failed") != std::string::npos,
            "allocation failure diagnostic");
    std::cout << "PASS: allocation failure returns a diagnostic and exit status 1" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
