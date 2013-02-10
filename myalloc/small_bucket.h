#ifndef _SMALL_BUCKET_H_
#define _SMALL_BUCKET_H_
#include "bucket.h"
bucket_t* create_small();
void free_small(bucket_t** list, void* ptr);
void destroy_small(bucket_t* bucket);
bucket_t* find_small_bucket(bucket_t** list);
size_t max_small_size();
#endif