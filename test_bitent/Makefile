all: test1.dat test2.dat test3.dat test3-long.dat

test1.dat:
	./test1

test2.dat: test1.dat
	./test2

test3.dat:
	./test3

test3-long.dat:
	./test3-long

clean:
	rm -f *.dat
