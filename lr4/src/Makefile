CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fPIC
LDFLAGS = 
DLFLAGS = -ldl

all: lib1.so lib2.so prog1 prog2

lib1.so: lib1.c libs.h
	$(CC) $(CFLAGS) -shared -o $@ lib1.c

lib2.so: lib2.c libs.h
	$(CC) $(CFLAGS) -shared -o $@ lib2.c

prog1: prog1.c lib1.so
	$(CC) $(CFLAGS) -o $@ prog1.c -L. -l1 -Wl,-rpath=.

prog2: prog2.c
	$(CC) $(CFLAGS) -o $@ prog2.c $(DLFLAGS)

clean:
	rm -f *.so prog1 prog2
