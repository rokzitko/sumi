all: test1a.dat test1b.dat test1c.dat test2b.dat test3a.dat test3b.dat test4.dat

test1a.dat: test1a
	./test1a

test1b.dat: test1b
	./test1b

test1c.dat: test1c
	./test1c

test2b.dat: test2b
	./test2b

test3a.dat: test3a-avg
	./test3a-avg

test3b.dat: test3b-avg
	./test3b-avg

test4.dat: test4
	./test4

clean:
	rm -f *.dat
