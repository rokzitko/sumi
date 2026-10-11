CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wshadow -Wpedantic
PYTHON ?= python3
SUMI ?= ../sumi

.PHONY: all check clean
all: check

check: check.out
	./check.out
	$(PYTHON) check.py --sumi $(SUMI)

check.out: check.cc ../filter.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) check.cc -o $@

clean:
	rm -f check.out
