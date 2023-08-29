CXXFLAGS=-std=c++20 -O3 -Wall
FFTW=-fopenmp -lfftw3_omp -lfftw3 -lm

all: sumi bitdump bitent

sumi: sumi.cc misc.h io.h common.h filter.h
	g++ ${CXXFLAGS} ${FFTW} sumi.cc -o sumi

bitdump: bitdump.cc misc.h io.h common.h readbits.h
	g++ ${CXXFLAGS} bitdump.cc -o bitdump

bitent: bitent.cc misc.h io.h common.h readbits.h
	g++ ${CXXFLAGS} bitent.cc -o bitent

clean:
	rm -f sumi bitdump bitent
