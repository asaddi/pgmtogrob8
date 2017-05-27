# Makefile for pgmtogrob8

# Edit these if you don't have gcc
CC = gcc
CFLAGS = -O2

# Edit this to point to the netpbm headers
INCLUDEDIR = -I/usr/sww/include

# Edit this to point to the netpbm libs
LIBDIR = -L/usr/sww/lib

# Required netpbm libraries
LIBS = -lpgm -lpbm

all: pgmtogrob8

pgmtogrob8: pgmtogrob8.o
	$(CC) $(LIBDIR) -o pgmtogrob8 pgmtogrob8.o $(LIBS)

pgmtogrob8.o: pgmtogrob8.c
	$(CC) -c $(INCLUDEDIR) $(CFLAGS) -o pgmtogrob8.o pgmtogrob8.c

test: pgmtogrob8
	@pgmtogrob8 test/skuld.pgm >test.out
	@if cmp test.out test/skuld.grob >/dev/null;\
	then\
		echo pgmtogrob8 passed;\
	else\
		echo pgmtogrob8 failed test;\
	fi
	@rm -f test.out

clean:
	rm -f *.o pgmtogrob8 test.out
