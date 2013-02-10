#ifndef _MYALLOC_H_
#define _MYALLOC_H_
void* malloc(size_t);
void free(void*);
/*----------NOT IMPLEMENTED-----------------*/
void* calloc(size_t, size_t);
void* realloc(void* old, size_t size);
void* memalign(size_t align, size_t size);
int posix_memalign(void** memptr, size_t align, size_t size);
void* valloc(size_t size);
#endif