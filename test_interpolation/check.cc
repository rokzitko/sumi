// Exercise the generator directly, retaining the CLI under a different name.
#define main sumi_main
#include "../sumi.cc"
#undef main

#include <numbers>
#include <stdexcept>

void require(bool condition, const std::string &message)
{
  if (!condition) throw std::runtime_error(message);
}

void check_timing(uint64_t bs)
{
  std::default_random_engine dre(1234);
  uint64_t calls = 0;
  auto S = [&calls](uint64_t f) {
    calls++;
    return 1.0/f;
  };
  auto gen = from_spectrum_with_interpolation(dre, S, bs);
  for (uint64_t i = 0; i <= 3*bs; i++) {
    const auto before = calls;
    require(std::isfinite(gen()), "non-finite sample");
    // timmer_konig_setup evaluates S twice per interior frequency and once at Nyquist.
    const uint64_t blocks = i == 0 ? 2 : (i % (bs/2) == 0 ? 1 : 0);
    require(calls-before == blocks*(bs-1), "regeneration count at sample " + std::to_string(i));
  }
}

void check_sequence(uint64_t bs, bool norm, double factor, std::function<double(uint64_t)> S)
{
  std::default_random_engine dre(1234), reference_dre(1234);
  auto gen = from_spectrum_with_interpolation(dre, S, bs, norm, factor);
  auto blocks = from_spectrum(reference_dre, S, bs, norm, factor);
  const int64_t size = bs;
  const int64_t count = 4*size+3;
  std::vector<double> expected(count, 0.0);

  // Independent, blockwise overlap-add reference. The first block starts before
  // sample zero. sqrt(Hann) is equivalent to the nonnegative sine window.
  for (int64_t start = -size/2; start < count; start += size/2) {
    for (int64_t j = 0; j < size; j++) {
      const double x = blocks();
      const int64_t i = start+j;
      if (i >= 0 && i < count) {
        const double w = sqrt((1.0-cos(2.0*std::numbers::pi*j/size))/2.0);
        expected[i] += w*x;
      }
    }
  }
  for (int64_t i = 0; i < count; i++) {
    const double x = gen();
    require(std::isfinite(x) && abs(x-expected[i]) <= 2e-11*(1.0+abs(expected[i])),
            "overlap-add mismatch at sample " + std::to_string(i));
  }
}

int main()
{
  int failures = 0;
  auto run = [&failures](const std::string &label, auto test) {
    try {
      test();
      std::cout << "PASS: " << label << std::endl;
    } catch (const std::exception &e) {
      failures++;
      std::cerr << "FAIL: " << label << ": " << e.what() << std::endl;
    }
  };
  const auto mixed = [](uint64_t f) { return 0.2/f+0.03; };
  for (const uint64_t bs : {2, 6, 16, 1024}) {
    const auto label = "bs=" + std::to_string(bs);
    run("timing " + label, [=] { check_timing(bs); });
    for (const bool norm : {false, true}) {
      run("reference " + label + " norm=" + std::to_string(norm),
          [=] { check_sequence(bs, norm, 1.0, mixed); });
    }
  }
  for (const bool norm : {false, true}) {
    run("pink norm=" + std::to_string(norm),
        [=] { check_sequence(16, norm, 1.0, [](uint64_t f) { return 1.0/f; }); });
    run("white norm=" + std::to_string(norm),
        [=] { check_sequence(16, norm, 1.0, [](uint64_t) { return 1.0; }); });
    for (const double factor : {0.0, -2.5}) {
      run("scaling norm=" + std::to_string(norm) + " factor=" + std::to_string(factor),
          [=] { check_sequence(16, norm, factor, mixed); });
    }
  }
  run("zero spectrum", [] { check_sequence(6, false, 1.0, [](uint64_t) { return 0.0; }); });
  return failures ? 1 : 0;
}
