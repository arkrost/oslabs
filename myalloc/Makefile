CC=gcc
CFLAGS=-shared -fPIC -nostdlib -ggdb

all: libmyalloc.so

libmyalloc.so: bucket.h large_bucket.h my_pthread.h myalloc.h small_bucket.h bucket.c large_bucket.c my_pthread.c myalloc.c small_bucket.c
	$(CC) $(CFLAGS) bucket.c large_bucket.c my_pthread.c myalloc.c small_bucket.c -o libmyalloc.so

clean:
	rm libmyalloc.so
