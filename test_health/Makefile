all: test1.dat test2a.dat test2b.dat test2c.dat test3.dat

test1.dat: test1
	./test1

test2a.dat: test2a
	./test2a

test2b.dat: test2b
	./test2b

test2c.dat: test2c
	./test2c

test3.dat: test3
	./test3

CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wshadow -Wpedantic
FFTW ?= $(shell pkg-config --cflags --libs fftw3)
PYTHON ?= python3
SUMI ?= ../sumi

.PHONY: check
check: check.out
	./check.out
	$(PYTHON) check.py --sumi $(SUMI)

check.out: check.cc ../sumi.cc ../version.h $(wildcard ../*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) check.cc -o $@ $(FFTW)

clean: 
	rm -f test1.dat test2a.dat test2b.dat test2c.dat test3.dat check.out
