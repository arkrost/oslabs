CC=gcc
CFLAGS=-shared -fPIC -nostdlib -g

all: libmyalloc.so

libmyalloc.so: bucket.h bucket.c mythread.h mythread.c myalloc.c
	$(CC) $(CFLAGS) bucket.c mythread.c myalloc.c -o libmyalloc.so

clean:
	rm libmyalloc.so