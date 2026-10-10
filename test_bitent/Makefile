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

.PHONY: check
check:
	$(PYTHON) check_entropy.py --bitent $(BITENT)

clean:
	rm -f *.dat
