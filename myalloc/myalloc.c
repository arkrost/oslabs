#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include "myalloc.h"
#define SMALL_MEM_LIMIT 128
#define SMALL_NUM_LIMIT 20
#define LARGE_NUM_LIMIT 10

/*
* Structures
*/
typedef struct bucket
{
	size_t len;
	pid_t tid;
	void* mem;
} bucket_t;

typedef struct threadlocal
{
	pid_t tid;
	pthread_mutex_t small_bucket_mutex;
	pthread_mutex_t large_bucket_mutex;
	bucket_t* small[SMALL_NUM_LIMIT];
	bucket_t* large[LARGE_NUM_LIMIT];
	struct threadlocal* next;
} threadlocal_t;

typedef struct global_memory
{
	bucket_t* mem;
	struct global_memory* next;
} global_memory_t;
/*
* Globals
*/
static pthread_mutex_t thread_list_mutex;
static pthread_mutex_t memory_list_mutex;
static threadlocal_t* thread_list = NULL;
static global_memory_t* memory_list = NULL;


/*
* Thread safe functions
*/
static bucket_t* new_small_bucket()
{
	bucket_t* res = (bucket_t*)mmap(NULL, sizeof(bucket_t) + SMALL_MEM_LIMIT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	res->len = SMALL_MEM_LIMIT;
	res->mem = (void*)res + sizeof(bucket_t);
	res->tid = pthread_self();
	return res;
}

static bucket_t* new_large_bucket(size_t size)
{
	// assert(size > SMALL_MEM_LIMIT);
	bucket_t* res = (bucket_t*)mmap(NULL, sizeof(bucket_t) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	res->len = size;
	res->mem = (void*)res + sizeof(bucket_t);
	res->tid = pthread_self();
	return res;
}

static void free_bucket(bucket_t* bucket)
{
	munmap((void*)bucket, sizeof(bucket_t) + bucket->len);	
}

static void free_threadlocal(threadlocal_t* tl) 
{
	if (tl == NULL) return;
	size_t i;
	for (i = 0; i < SMALL_NUM_LIMIT; i++) 
	{
		free_bucket(tl->small[i]);
	}
	for (i = 0; i < LARGE_NUM_LIMIT; i++) 
	{
		free_bucket(tl->large[i]);
	}
	pthread_mutex_destroy(&(tl->small_bucket_mutex));
	pthread_mutex_destroy(&(tl->large_bucket_mutex));
	free_threadlocal(tl->next);
	munmap((void*)tl, sizeof(threadlocal_t));
}

static void free_global_memory(global_memory_t* gm)
{
	if (gm == NULL) return;
	free_bucket(gm->mem);
	free_global_memory(gm->next);
	munmap((void*)gm, sizeof(global_memory_t));
}

/*
* Thread unsafe functions
*/
threadlocal_t* get_threadlocal(pid_t tid) 
{
	threadlocal_t* cur = thread_list;
	while (cur) 
	{
		if (cur->tid == tid) break;
		cur = cur->next;
	}
	if (cur == NULL) 
	{
		cur = (threadlocal_t*)mmap(NULL, sizeof(threadlocal_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		cur->tid = tid;
		memset(cur->small, 0, sizeof(bucket_t*)*SMALL_NUM_LIMIT);
		memset(cur->large, 0, sizeof(bucket_t*)*LARGE_NUM_LIMIT);
		pthread_mutex_init(&(cur->small_bucket_mutex), 0);
		pthread_mutex_init(&(cur->large_bucket_mutex), 0);
		cur->next = thread_list;
		thread_list = cur;
	} 
	return cur;
}


/*
* Header functions
*/
void* malloc(size_t size) 
{
    if (size > SMALL_MEM_LIMIT) 
    {
    	bucket_t* res = new_large_bucket(size);
    	return res->mem;
    }
    else 
    {
    	bucket_t* res = new_small_bucket();
    	return res->mem;	
    }
}

void free(void* ptr) 
{
	if (ptr == NULL) return;
	free_bucket((bucket_t*)(ptr - sizeof(bucket_t)));
}

void* calloc(size_t nmemb, size_t size) 
{
    size_t n = nmemb * size;
    void* res = malloc(n);
    res = memset(res, 0, n);
    return res;
}

void* realloc(void* ptr, size_t size) 
{
    if (ptr == NULL) return malloc(size);
    if (size == 0) 
    {
        free(ptr);
        return NULL;
    }
    size_t old_size = ((bucket_t*)(ptr - sizeof(bucket_t)))->len;
    void* res = malloc(size);
    memcpy(res, ptr, old_size < size ? old_size : size);
    free(ptr);
    return res;
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
	pthread_mutex_init(&thread_list_mutex, 0);
	pthread_mutex_init(&memory_list_mutex, 0);
}

void _fini() 
{
	pthread_mutex_destroy(&thread_list_mutex);
	pthread_mutex_destroy(&memory_list_mutex);
	free_threadlocal(thread_list);
	free_global_memory(memory_list);
}
