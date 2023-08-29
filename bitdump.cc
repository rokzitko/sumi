// Dump time series of bit. Can also perform various bit-order (LSB to MSB vs. MSB to LSB), wordsize and byte-order (endianness) conversions.
// Rok Zitko, 2023

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <random>
#include <bitset>
#include <stdexcept>
#include <ctime>
#include <cassert>
#include <coroutine>
#include <math.h>
#include <unistd.h>

#include <complex>

#include "misc.h"
#include "io.h"
#include "readbits.h"

bool verbose;
auto & msg = std::cerr; // stream for diagnostic messages

int main(int argc, char *argv[])
{
  InputParser input(argc, argv);

  const std::string filename { input.exists("-i") ? input.get("-i") : "" };
  const int ws { input.exists("-ws") ? std::stoi(input.get("-ws")) : 8 }; // word size in bits (8, 16, 32, 64)
  const Direction id { input.exists("-id") ? std::stoi(input.get("-id")) : 1 }; // default is 1 (lsb to msb)
  const bool iw = input.exists("-iw"); // byte swap for binary input (endianness change)
  const uint64_t count { input.exists("-c") ? std::stoul(input.get("-c")) : 0 }; // number of random numbers generated, 0 = infinite
  const OutputType ot { input.exists("-ot") ? std::stoi(input.get("-ot")) : 1 }; // default is bits as integers (ASCII)
  const Direction od { input.exists("-od") ? std::stoi(input.get("-od")) : 1 }; // default is 1 (lsb to msb)
  const bool ow = input.exists("-ow"); // byte swap for binary output (endianness change)
  verbose = input.exists("-v");

  if (verbose) {
    msg << "filename=" << (filename != "" ? filename : "[stdin]") << std::endl;
    msg << "input word size ws=" << ws << std::endl;
    msg << "input bit direction id=" << static_cast<int>(id) << " [" << name(id) << "]" << std::endl;
    msg << "input endianness swap iw=" << std::boolalpha << iw << std::endl;
    showcount(msg, count);
    msg << "output type ot=" << static_cast<int>(ot) << " " << name(ot) << std::endl;
    msg << "output bit diretction od=" << static_cast<int>(od) << " [" << name(od) << "]" << std::endl;
    msg << "output endianness swap ow=" << std::boolalpha << ow << std::endl;
  }

  auto gen = boolreader(filename, ws, id, iw);
  int ndx = 0;
  std::bitset<8> b8;
  std::bitset<16> b16;
  std::bitset<32> b32;
  std::bitset<64> b64;
  std::cout << std::noboolalpha;
  uint64_t i;
  for (i = 0; i < count || count == 0; i++) {
   try {
    bool b = (*gen)();
    switch (ot) {
    case OutputType::lsb:
      std::cout << b << std::endl;
      break;
    case OutputType::lsb_8:
      b8[od == Direction::lm ? ndx : 7-ndx] = b;
      ndx++;
      if (ndx == 8) {
        writebs<uint8_t, 8>(b8, ow);
        ndx = 0;
      }
      break;
    case OutputType::lsb_16:
      b16[od == Direction::lm ? ndx : 15-ndx] = b;
      ndx++;
      if (ndx == 16) {
        writebs<uint16_t, 16>(b16, ow);
        ndx = 0;
      }
      break;
    case OutputType::lsb_32:
      b32[od == Direction::lm ? ndx : 31-ndx] = b;
      ndx++;
      if (ndx == 32) {
        writebs<uint32_t, 32>(b32, ow);
        ndx = 0;
      }
      break;
    case OutputType::lsb_64:
      b64[od == Direction::lm ? ndx : 63-ndx] = b;
      ndx++;
      if (ndx == 64) {
        writebs<uint64_t, 64>(b64, ow);
        ndx = 0;
      }
      break;
    case OutputType::none:
      break;
    default:
      throw std::runtime_error("Output type -ot should be 1, 8, 16, 32, 64 or 99.");
    }
   } catch (const EOF_exception &) {
     if (verbose)
       msg << "nr bits read=" << i << std::endl;
     break;
   } catch (const std::exception& e) {
     std::cerr << "Error: " << e.what() << std::endl;
     exit(1);
   }
  }
}
