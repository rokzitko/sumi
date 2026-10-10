all: test1.dat test1a.dat test2.dat test2a.dat

test1.dat: test1
	./test1

test1a.dat: test1a
	./test1a

test2.dat: test2
	./test2

test2a.dat: test2a
	./test2a

test3.dat: test3
	./test3

CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wshadow -Wpedantic

.PHONY: check
check: check_variance.out
	./check_variance.out

check_variance.out: check_variance.cc ../stats.h ../common.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) check_variance.cc -o $@

clean: 
	rm -f test1.dat test1a.dat test2.dat test2a.dat test3.dat check_variance.out

