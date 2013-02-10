#ifndef _BUCKET_H_
#define _BUCKET_H_
#include <pthread.h>

typedef struct bucket 
{
    size_t extra;
    void* mem;
    struct bucket* next;
    struct bucket* prev;
    pthread_t id;
} bucket_t;

void delete_bucket(bucket_t** list, bucket_t* bucket);
bucket_t* create_bucket(size_t size);
void add_bucket(bucket_t** list, bucket_t* bucket);
#endif