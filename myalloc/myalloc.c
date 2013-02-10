#include <signal.h>

void* malloc(size_t size) {
  raise(SIGILL);
}

void* calloc(size_t elems, size_t bytes_each) {
  raise(SIGILL);
}

void free(void* ptr) {
  raise(SIGILL);
}

void* realloc(void* old, size_t size) {
  raise(SIGILL);
}

void* memalign(size_t align, size_t size) {
  raise(SIGILL);
}

int posix_memalign(void** memptr, size_t align, size_t size) {
  raise(SIGILL);
}

void* valloc(size_t size) {
  raise(SIGILL);
}