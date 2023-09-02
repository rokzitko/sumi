#ifndef _io_h_
#define _io_h_

#include <iostream>
#include <fstream>
#include <bitset>
#include <cassert>
#include <unistd.h>

#include "common.h"

static_assert(sizeof(unsigned long) == sizeof(uint64_t), "64-bit platform required");

template<typename T, std::size_t size>
void writebs(std::bitset<size> b, bool bswap = false) {
  const auto val = static_cast<T>(b.to_ulong());
  const T tmp = bswap ? byteswap(val) : val;
  std::cout.write((char*)&tmp, sizeof(T));
}

void write(std::bitset<64> b, std::size_t size, bool bswap = false) {
  switch (size) {
  case 8:
    return writebs<uint8_t,  64>(b, bswap);
  case 16:
    return writebs<uint16_t, 64>(b, bswap);
  case 32:
    return writebs<uint32_t, 64>(b, bswap);
  case 64:
    return writebs<uint64_t, 64>(b, bswap);
  default:
    std::cerr << "size should be 8, 16, 32 or 64, got " << size << std::endl;
    assert(0);
  }
}

#endif
