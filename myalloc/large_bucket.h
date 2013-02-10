#ifndef _LARGE_BUCKET_H_
#define _LARGE_BUCKET_H_
#include "bucket.h"
void append_bucket(bucket_t** list, bucket_t* bucket);
bucket_t* find_large_bucket(bucket_t** list, size_t size);
void destroy_large(bucket_t* bucket);
#endif