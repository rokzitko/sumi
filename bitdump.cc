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

#include "version.h"
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
  auto ows = get_ws(ot);
  const Direction od { input.exists("-od") ? std::stoi(input.get("-od")) : 1 }; // default is 1 (lsb to msb)
  const bool ow = input.exists("-ow"); // byte swap for binary output (endianness change)
  verbose = input.exists("-v");

  if (verbose) {
    msg << "bitdump " << GIT_HASH << " " << __DATE__ << " " << __TIME__ << std::endl;
    msg << "filename=" << (filename != "" ? filename : "[stdin]") << std::endl;
    msg << "input word size ws=" << ws << std::endl;
    msg << "input bit direction id=" << static_cast<int>(id) << " [" << name(id) << "]" << std::endl;
    msg << "input endianness swap iw=" << std::boolalpha << iw << std::endl;
    show_with_logs(msg, "count", count, true);
    msg << "output type ot=" << static_cast<int>(ot) << " " << name(ot) << std::endl;
    msg << "output bit diretction od=" << static_cast<int>(od) << " [" << name(od) << "]" << std::endl;
    msg << "output endianness swap ow=" << std::boolalpha << ow << std::endl;
  }

  auto gen = boolreader(filename, ws, id, iw);
  std::size_t ndx = 0;
  std::bitset<64> bits;
  std::cout << std::noboolalpha;
  uint64_t i;
  for (i = 0; i < count || count == 0; i++) {
    try {
      bool b = (*gen)();
      using enum OutputType;
      switch (ot) {
      case lsb:
        std::cout << b << std::endl;
        break;
      case lsb_8:
      case lsb_16:
      case lsb_32:
      case lsb_64:
        bits[od == Direction::lm ? ndx : ows-1-ndx] = b;
        ndx++;
        if (ndx == ows) {
          write(bits, ows, ow);
          ndx = 0;
        }
        break;
      case none:
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
