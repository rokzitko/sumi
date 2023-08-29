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

#endif
