#include "mythread.h"
#include <sys/mman.h>

#define HASH_SIZE 20000

mythread_t* pthreads[HASH_SIZE] = {};

mythread_t* get_pthread(pthread_t id) 
{
    if (pthreads[id % HASH_SIZE]) return pthreads[id % HASH_SIZE];
    mythread_t* cur = (mythread_t*)mmap(0, sizeof(mythread_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cur->id = id;
    cur->large = NULL;
    cur->small = NULL;
    cur->free_small = 0;
    cur->free_large = 0;
    pthread_mutex_init(&cur->local_mutex, 0);
    pthreads[id % HASH_SIZE] = cur;
    return cur;
}


void destroy_thread(mythread_t* thread) 
{
    if (!thread) return;
    pthread_mutex_destroy(&thread->local_mutex);
    destroy_small(thread->small);
    destroy_large(thread->large);
    pthreads[thread->id % HASH_SIZE] = NULL;
    munmap((void*)thread, sizeof(mythread_t));
}

void destroy_all() 
{
    size_t i;
    for (i = 0; i < HASH_SIZE; i++) 
    {
        destroy_thread(pthreads[i]);
    }
}
