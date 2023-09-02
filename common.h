#ifndef _common_h_
#define _common_h_

#include <cassert>

// Consecutive bit ordering within words
enum class Direction {
  lm = 1, // LSB to MSB
  ml = 2  // MSB to LSB
};

std::string name(Direction d)
{
  using enum Direction;
  switch (d) {
  case lm:
    return "LSB to MSB";
  case ml:
    return "MSB to LSB";
  default:
    assert(0);
  }
}

// Functions for reversing bytes. Note: most modern CPUs are little-endian.
inline uint8_t  byteswap(const uint8_t  x) { return x; }
inline uint16_t byteswap(const uint16_t x) { return __builtin_bswap16(x); }
inline uint32_t byteswap(const uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t byteswap(const uint64_t x) { return __builtin_bswap64(x); }

void show_with_logs(std::ostream &msg, const std::string label, const uint64_t val, bool zero_is_infinite = false)
{
  msg << label << "=" << val;
  if (val)
    msg << "=10^" << std::setprecision(4) << log10(val) << "=2^" << std::setprecision(4) << log2(val);
  else
    if (zero_is_infinite)
      msg << " [0=infinite]";
  msg << std::endl;
}

enum class OutputType {
  floating = -1,
  integer = 0,
  lsb = 1,
  lsb_8 = 8,
  lsb_16 = 16,
  lsb_32 = 32,
  lsb_64 = 64,
  none = 99
};

std::string name(OutputType ot) {
  using enum OutputType;
  switch (ot) {
  case floating:
    return "floating point";
  case integer:
    return "integer";
  case lsb:
    return "LSB (integer)";
  case lsb_8:
    return "LSB (binary, 8-bit word)";
  case lsb_16:
    return "LSB (binary, 16-bit word)";
  case lsb_32:
    return "LSB (binary, 32-bit word)";
  case lsb_64:
    return "LSB (binary, 64-bit word)";
  case none:
    return "no output";
  default:
    assert(0);
  }
}

std::size_t get_ws(const OutputType ot) {
  using enum OutputType;
  switch (ot) {
  case lsb_8:
    return 8;
  case lsb_16:
    return 16;
  case lsb_32:
    return 32;
  case lsb_64:
    return 64;
  default:
    return 0;
  }
}

#endif
