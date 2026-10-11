#include <cmath>
#include <concepts>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../common.h"
#include "../misc.h"

void require(bool condition, const std::string &message)
{
  if (!condition) throw std::runtime_error(message);
}

InputParser parser(std::initializer_list<std::string> options)
{
  std::vector<std::string> args {"test"};
  args.insert(args.end(), options.begin(), options.end());
  std::vector<char*> argv;
  for (auto &arg : args) argv.push_back(arg.data());
  return InputParser(static_cast<int>(argv.size()), argv.data());
}

template <typename F>
void rejected(F f)
{
  try {
    f();
  } catch (const std::invalid_argument &) {
    return;
  }
  throw std::runtime_error("Invalid argument was accepted.");
}

int main()
{
  try {
    auto input = parser({"-signed", "-3", "-unsigned", "+0008", "-real", "-1.25e-2",
                         "-i", "./-v", "-v", "-v"});
    input.validate_options({"-signed", "-unsigned", "-real", "-i"}, {"-v"});
    require(input.get_int("-signed", 0) == -3, "signed integer");
    require(input.get_uint64("-unsigned", 0) == 8, "unsigned integer");
    require(input.get_double("-real", 0) == -0.0125, "signed scientific notation");
    require(input.get("-i") == "./-v" && input.exists("-v"), "filename and repeated switch");
    require(input.get_int("-missing", -7) == -7, "signed default");
    require(input.get_uint64("-missing", 9) == 9, "unsigned default");
    require(input.get_double("-missing", 0.25) == 0.25, "real default");
    require(input.get_choice("-missing", 2, {1, 2}) == 2, "choice default");
    require(parser({"-i", ""}).get("-i").empty(), "explicit stdin filename");
    require(std::signbit(parser({"-real", "-0"}).get_double("-real", 1)), "negative zero");

    for (const auto value : {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()})
      require(parser({"-n", std::to_string(value)}).get_int("-n", 0) == value, "int endpoint");
    const auto maximum = std::numeric_limits<uint64_t>::max();
    require(parser({"-n", std::to_string(maximum)}).get_uint64("-n", 0) == maximum, "uint64 endpoint");
    require(parser({"-n", "0"}).get_uint64("-n", 1) == 0, "unsigned zero");
    std::cout << "PASS: defaults, signed values, scientific notation, and numeric endpoints" << std::endl;

    for (const auto &args : {std::vector<std::string>{"-unknown"}, {"argument"}, {"-n"},
                             {"-n", "-v"}, {"-n", "1", "-n", "2"}, {"-v", "argument"}}) {
      std::vector<std::string> values {"test"};
      values.insert(values.end(), args.begin(), args.end());
      std::vector<char*> argv;
      for (auto &value : values) argv.push_back(value.data());
      InputParser invalid(static_cast<int>(argv.size()), argv.data());
      rejected([&] { invalid.validate_options({"-n"}, {"-v"}); });
    }
    rejected([] { parser({"-i", "-v"}).validate_options({"-i"}, {"-v"}); });
    parser({"-i", ""}).validate_options({"-i"}, {"-v"});
    std::cout << "PASS: option arity, duplicates, and positional-argument checks" << std::endl;

    for (const auto &value : {"", "abc", "1x", "1.5", "999999999999999999999999999999999999"}) {
      rejected([&] { parser({"-n", value}).get_int("-n", 0); });
      rejected([&] { parser({"-n", value}).get_uint64("-n", 0); });
    }
    for (const auto &value : {"-1", "-18446744073709551615", "18446744073709551616"})
      rejected([&] { parser({"-n", value}).get_uint64("-n", 0); });
    for (const auto &value : {"", "abc", "1.5x", "nan", "inf", "-inf", "1e9999"})
      rejected([&] { parser({"-n", value}).get_double("-n", 0); });
    rejected([] { parser({"-n", "3"}).get_choice("-n", 1, {1, 2}); });
    rejected([] { name(static_cast<Direction>(3)); });
    rejected([] { name(static_cast<OutputType>(7)); });
    std::cout << "PASS: invalid numeric values, choices, and enum names" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
