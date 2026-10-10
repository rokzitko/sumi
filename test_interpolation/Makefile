CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wshadow -Wpedantic
FFTW ?= $(shell pkg-config --cflags --libs fftw3)
PYTHON ?= python3
SUMI ?= ../sumi

.PHONY: all check clean
all: check

check: check.out
	./check.out
	$(PYTHON) check.py --sumi $(SUMI)

check.out: check.cc ../sumi.cc ../version.h $(wildcard ../*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) check.cc -o $@ $(FFTW)

../version.h:
	$(MAKE) -C .. version.h

clean:
	rm -f check.out
