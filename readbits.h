#ifndef _readbits_h_
#define _readbits_h_

#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <stdexcept>
#include <coroutine>

#include "common.h"

class EOF_exception : public std::exception {
 public:
   const char *what () {
     return "End of file reached";
   }
};

template<typename T, int len>
Generator<bool> read(std::string filename, Direction id, bool bswap = false)
{
  std::ifstream F_file;
  if (filename != "") {
    F_file.open(filename);
    if (!F_file) throw std::runtime_error( "Can't open file [" + filename + "] for reading.");
  }
  std::istream & F = (filename != "" ? F_file : std::cin); // if filename is not given, read from stdin
  while (F) {
    T x;
    F.read((char*)&x, sizeof(T));
    T val = bswap ? byteswap(x) : x;
    std::bitset<len> bs { val };
    if (F) {
      for (int i = 0; i < len; i += 1)
        co_yield bs[ id == Direction::lm ? i : len-1-i ];
    } else throw EOF_exception();
  }
}

auto boolreader(std::string filename, int ws, Direction id, bool bswap)
{
  Generator<bool> *gen;
  switch (ws) {
  case 8:
        gen = new Generator<bool>(read<uint8_t,   8>(filename, id, bswap));
        break;
  case 16:
        gen = new Generator<bool>(read<uint16_t, 16>(filename, id, bswap));
        break;
  case 32:
        gen = new Generator<bool>(read<uint32_t, 32>(filename, id, bswap));
        break;
  case 64:
        gen = new Generator<bool>(read<uint64_t, 64>(filename, id, bswap));
        break;
  default:
    throw std::runtime_error("Supported word sizes are 8, 16, 32, 64.");
  }
  return gen;
}

#endif
