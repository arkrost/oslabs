CC=gcc
CFLAGS=-shared -fPIC -nostdlib -ggdb

all: libmyalloc.so

libmyalloc.so: myalloc.h myalloc.c
	$(CC) $(CFLAGS) myalloc.c -o libmyalloc.so

clean:
	rm libmyalloc.so