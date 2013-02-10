#ifndef _MY_PTHREAD_
#define _MY_PTHREAD_
#include "bucket.h"
#include <pthread.h> 
typedef struct {
    pthread_t id;
    bucket_t* large;
    bucket_t* small;
    size_t free_large;
    size_t free_small;
    pthread_mutex_t local_mutex;
} my_pthread_t;

my_pthread_t* get_pthread(pthread_t id);
void destroy_thread(my_pthread_t* thread);
void destroy_all();
#endif