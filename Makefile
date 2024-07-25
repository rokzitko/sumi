CXXFLAGS=-std=c++20 -O3 -Werror -Wall -Wextra -Wshadow -Wpedantic -Wno-clobbered
FFTW=-fopenmp -lfftw3_omp -lfftw3 -lm
CXX=g++

all: sumi bitdump bitent

GIT_HASH := $(shell git describe --always)

version.h: .git/index
	echo "#define GIT_HASH \"${GIT_HASH}\"" > $@

sumi: sumi.cc misc.h io.h common.h filter.h version.h stats.h
	${CXX} ${CXXFLAGS} ${FFTW} sumi.cc -o sumi -lfftw3

bitdump: bitdump.cc misc.h io.h common.h readbits.h version.h
	${CXX} ${CXXFLAGS} bitdump.cc -o bitdump

bitent: bitent.cc misc.h io.h common.h readbits.h version.h
	${CXX} ${CXXFLAGS} bitent.cc -o bitent

clean:
	rm -f sumi bitdump bitent
