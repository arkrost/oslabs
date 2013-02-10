#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "myalloc.h"
#define SMALL_MEM_LIMIT 32

/*
* Structures
*/

typedef struct bucket
{
	size_t len;
	void* mem;
	struct bucket* next;
	struct bucket* prev;
} bucket_t;

typedef struct threadlocal
{
	pid_t tid;
	struct threadlocal* next;
} threadlocal_t;

/*
* Globals
*/
static pthread_mutex_t threadListMutex;
static pthread_mutex_t memoryListMutex;
static threadlocal_t* threadList = NULL;
static bucket_t* memoryList = NULL;

void _init() 
{
	pthread_mutex_init(&threadListMutex, 0);
	pthread_mutex_init(&memoryListMutex, 0);
}

void _fini() 
{
	pthread_mutex_destroy(&threadListMutex);
	pthread_mutex_destroy(&memoryListMutex);
}

/*
* Thread safe functions
*/
static bool is_small(size_t size)
{
	return size <= SMALL_MEM_LIMIT;
}

static bucket_t* new_small_bucket()
{
	return (bucket_t*)mmap(NULL, sizeof(bucket_t) + SMALL_MEM_LIMIT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

static bucket_t* new_large_bucket(size_t size)
{
	assert(size > SMALL_MEM_LIMIT);
	return (bucket_t*)mmap(NULL, sizeof(bucket_t) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
/*
* Thread unsafe functions
*/
threadlocal_t* getThreadLocal() 
{
	pid_t tid = pthread_self();
	threadlocal_t* cur = threadList;
	while (cur) 
	{
		if (cur->tid == tid) break;
		cur = cur->next;
	}
	if (cur == NULL) 
	{
		cur = (threadlocal_t*)mmap(NULL, sizeof(threadlocal_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		cur->next = threadList;
		threadList = cur;
	} 
	return cur;
}


/*
* Header functions
*/
void* malloc(size_t size) 
{
    raise(SIGILL);
}

void free(void* ptr) 
{
    raise(SIGILL);
}

void* calloc(size_t nmemb, size_t size) 
{
    raise(SIGILL);
}

void* realloc(void* old, size_t size) 
{
    raise(SIGILL);
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