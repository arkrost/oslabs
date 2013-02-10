#include "bucket.h"
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>

void delete_bucket(bucket_t** list, bucket_t* bucket) 
{
    if ((list == NULL) || (bucket == NULL)) return;
    if ((*list) == bucket) 
    {
        *list = bucket->next;
        if (*list) (*list)->prev = NULL;
        bucket->next = NULL;
    } 
    else 
    {
        if (bucket->prev) bucket->prev->next = bucket->next;
        if (bucket->next) bucket->next->prev = bucket->prev;
    }
}

bucket_t* create_bucket(size_t size) 
{
    bucket_t* bucket = (bucket_t*)mmap(0, size + sizeof(bucket_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    bucket->mem = ((void*)bucket) + sizeof(bucket_t);
    bucket->extra = 0;
    bucket->next = NULL;
    bucket->prev = NULL;
    bucket->id = pthread_self();
    return bucket;
}

void add_bucket(bucket_t** list, bucket_t* bucket) 
{
    if (!bucket) return;
    if (*list) (*list)->prev = bucket;
    bucket->next = *list;
    *list = bucket;
}