#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "my_pthread.h"
#include "bucket.h"
#include "small_bucket.h"
#include "large_bucket.h"

bucket_t* global = NULL;
size_t global_size = 0;
pthread_mutex_t global_mutex;

void* malloc(size_t size) 
{
    if (size == 0)  return NULL;
    my_pthread_t* thread = get_pthread(pthread_self());
    bucket_t* list = thread->large;
    pthread_mutex_lock(&thread->local_mutex);
    bucket_t* b = find_large_bucket(&list, size);
    pthread_mutex_unlock(&thread->local_mutex);
    if (b)return b->mem;
    pthread_mutex_lock(&global_mutex);
    b = find_large_bucket(&global, size);
    pthread_mutex_unlock(&global_mutex);
    if (b) return b->mem;
    b = create_bucket(size);
    return b->mem;
}

void free(void* ptr) 
{
    if (ptr == NULL) return;
    bucket_t* b = (bucket_t*)(ptr - sizeof(bucket_t));
    munmap((void*)b, sizeof(bucket_t) + b->extra);
}

void* calloc(size_t nmemb, size_t size) 
{
    void* res = malloc(nmemb * size);
    res = memset(res, 0, nmemb  * size);
    return res;
}

    
static size_t get_size(void* ptr) {
    if (ptr) 
    {
        bucket_t* b = (bucket_t*)(ptr - sizeof(bucket_t));
        return b->extra;
    }
    return 0;
}

void* realloc(void* old, size_t size) 
{
    void* ptr = malloc(size);
    if (old) 
    {
        size_t old_size = get_size(old);
        if (ptr) 
        {
            memcpy(ptr, old, old_size < size ? old_size : size);
        }
        free(old);
    }
    return ptr;
}

void* memalign(size_t align, size_t size) 
{
    raise(SIGILL);
}

int posix_memalign(void** memptr, size_t align, size_t size) 
{
    raise(SIGILL);
}

void* valloc(size_t size) 
{
    raise(SIGILL);
}

void _init() 
{
    pthread_mutex_init(&global_mutex, 0);
}

void _fini() 
{
    pthread_mutex_destroy(&global_mutex);
    destroy_all();
    destroy_large(global);
}