CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wshadow -Wpedantic
PYTHON ?= python3
SUMI ?= ../sumi
BITDUMP ?= ../bitdump
BITENT ?= ../bitent

.PHONY: all check clean
all: check

check: check.out
	./check.out
	$(PYTHON) check.py --sumi $(SUMI) --bitdump $(BITDUMP) --bitent $(BITENT)

check.out: check.cc ../misc.h ../common.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) check.cc -o $@

clean:
	rm -f check.out
