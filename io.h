#ifndef _io_h_
#define _io_h_

#include <iostream>
#include <fstream>
#include <bitset>
#include <cassert>
#include <unistd.h>

#include "common.h"

static_assert(sizeof(unsigned long) == sizeof(uint64_t), "64-bit platform required");

template<typename T, int size>
void writebs(std::bitset<size> b, bool bswap = false) {
  const auto val = static_cast<T>(b.to_ulong());
  const T tmp = bswap ? byteswap(val) : val;
  std::cout.write((char*)&tmp, sizeof(T));
}

#endif
