#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include "myalloc.h"

#define SMALL_MEM_LIMIT 1024
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
	struct bucket* next;
} bucket_t;

typedef struct threadlocal
{
	pid_t tid;
	pthread_mutex_t small_bucket_mutex;
	pthread_mutex_t large_bucket_mutex;
	bucket_t* small_list;
	bucket_t* large_list;
	struct threadlocal* next;
} threadlocal_t;
/*
* Globals
*/
static pthread_mutex_t thread_list_mutex;
static pthread_mutex_t memory_list_mutex;
static threadlocal_t* thread_list = NULL;
static bucket_t* memory_list = NULL;


/*
* HELPERS
*/
static bucket_t* new_bucket(size_t size)
{
	bucket_t* res = (bucket_t*)mmap(NULL, sizeof(bucket_t) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (res)
	{
		res->len = size;
		res->mem = (void*)res + sizeof(bucket_t);
		res->next = NULL;
		res->tid = pthread_self();
	}
	return res;
}

static void free_bucket(bucket_t* bucket)
{
	if (bucket == NULL) return;
	free_bucket(bucket->next);
	munmap((void*)bucket, sizeof(bucket_t) + bucket->len);	
}

static void free_threadlocal(threadlocal_t* tl) 
{
	if (tl == NULL) return;
	free_bucket(tl->small_list);
	free_bucket(tl->large_list);
	pthread_mutex_destroy(&(tl->small_bucket_mutex));
	pthread_mutex_destroy(&(tl->large_bucket_mutex));
	free_threadlocal(tl->next);
	munmap((void*)tl, sizeof(threadlocal_t));
}

static threadlocal_t* get_threadlocal(pid_t tid) 
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
		cur->small_list = NULL;
		cur->large_list = NULL;
		pthread_mutex_init(&(cur->small_bucket_mutex), 0);
		pthread_mutex_init(&(cur->large_bucket_mutex), 0);
		cur->next = thread_list;
		thread_list = cur;
	} 
	return cur;
}

static size_t bucket_list_size(bucket_t* bucket)
{
	size_t count;
	for(count = 0; bucket != NULL; count++, bucket=bucket->next);
	return count;
}

/*
* Header functions
*/
void* malloc(size_t size) 
{
	pthread_mutex_lock(&thread_list_mutex); 
	threadlocal_t* tl = get_threadlocal(pthread_self());
	pthread_mutex_unlock(&thread_list_mutex); 
	if (tl == NULL) return NULL;
	bucket_t* res = NULL;
    if (size > SMALL_MEM_LIMIT) 
    {
    	bucket_t* prev;
    	// chech local cache
    	pthread_mutex_lock(&(tl->large_bucket_mutex));
    	for (res = tl->large_list, prev = NULL; (res != NULL) && (res->len < size); prev = res, res = res->next);
    	if (res)
    	{
    		if (prev)
    			prev->next = res->next;
    		else
    			tl->large_list = res->next;
    		res->next = NULL;
    	}
    	pthread_mutex_unlock(&(tl->large_bucket_mutex));
    	if (res) return res->mem;
    	// check global cache
    	pthread_mutex_lock(&memory_list_mutex);
    	for (res = memory_list, prev = NULL; (res != NULL) && (res->len < size); prev = res, res = res->next);
    	if (res)
    	{
    		if (prev)
    			prev->next = res->next;
    		else
    			memory_list = res->next;
    		res->tid = pthread_self();
    		res->next = NULL;
    	}
    	pthread_mutex_unlock(&memory_list_mutex);
    	if (res) return res->mem;
    	// allocate new
    	res = new_bucket(size);
    	return res->mem;
    }
    else 
    {
    	// check local cache
    	pthread_mutex_lock(&(tl->small_bucket_mutex));
		if (tl->small_list) 
		{
			res = tl->small_list;
			tl->small_list = res->next;
			res->next = NULL;
		}
    	pthread_mutex_unlock(&(tl->small_bucket_mutex));
    	if (res) return res->mem;
    	// allocate new
    	res = new_bucket(SMALL_MEM_LIMIT);
    	return res->mem;	
    }
}

void free(void* ptr) 
{
	if (ptr == NULL) return;
	bucket_t* bucket = (bucket_t*)(ptr - sizeof(bucket_t));
	pthread_mutex_lock(&thread_list_mutex); 
	threadlocal_t* tl = get_threadlocal(bucket->tid);
	pthread_mutex_unlock(&thread_list_mutex); 
	if (tl == NULL) return;
	if (bucket->len > SMALL_MEM_LIMIT)
	{
		// release to thread memory cache
		pthread_mutex_lock(&(tl->large_bucket_mutex));
		if(bucket_list_size(tl->large_list) < LARGE_NUM_LIMIT)
		{
			bucket->next=tl->large_list;
			tl->large_list = bucket;
			bucket = NULL;
		}
		pthread_mutex_unlock(&(tl->large_bucket_mutex));
		if (bucket == NULL) return;
		// release to global cache
		pthread_mutex_lock(&memory_list_mutex);
   		bucket->next = memory_list;
   		memory_list = bucket;
    	pthread_mutex_unlock(&memory_list_mutex);
	}
	else 
	{
		// release to thread memory cache
		pthread_mutex_lock(&(tl->small_bucket_mutex));
		if(bucket_list_size(tl->small_list) < SMALL_NUM_LIMIT)
		{
			bucket->next=tl->small_list;
			tl->small_list = bucket;
			bucket = NULL;
		}
		pthread_mutex_unlock(&(tl->small_bucket_mutex));
		// free if thread's cache is full
		if (bucket)	free_bucket(bucket);
	}
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
	free_bucket(memory_list);
}
