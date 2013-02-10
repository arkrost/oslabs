#ifndef _MYTHREAD_
#define _MYTHREAD_
#include "bucket.h"
#include <pthread.h> 
typedef struct 
{
    pthread_t id;
    bucket_t* large;
    bucket_t* small;
    size_t free_large;
    size_t free_small;
    pthread_mutex_t local_mutex;
} mythread_t;

mythread_t* get_pthread(pthread_t id);
void destroy_thread(mythread_t* thread);
void destroy_all();
#endif /* _MYTHREAD_ */