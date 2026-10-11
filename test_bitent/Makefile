all: test1.dat test2.dat test3.dat test3-long.dat

test1.dat:
	./test1

test2.dat: test1.dat
	./test2

test3.dat:
	./test3

test3-long.dat:
	./test3-long

PYTHON ?= python3
BITENT ?= ../bitent
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wshadow -Wpedantic

.PHONY: check
check: check_sizes.out
	./check_sizes.out
	$(PYTHON) check_entropy.py --bitent $(BITENT)
	$(PYTHON) check_moments.py --bitent $(BITENT)
	$(PYTHON) check_runs.py --bitent $(BITENT)

check_sizes.out: check_sizes.cc ../bitent.cc ../version.h $(wildcard ../*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) check_sizes.cc -o $@

clean:
	rm -f *.dat check_sizes.out
