#include "large_bucket.h"

void append_bucket(bucket_t** list, bucket_t* bucket) {
    if ((list == NULL) || (bucket == NULL)) return;
    bucket_t* cur = *list;
    while (cur != NULL) {
        if (cur->mem + cur->extra == bucket) {
            cur->extra += bucket->extra + sizeof(bucket_t);
            return;
        }
        if (bucket->mem + bucket->extra == cur) {
            bucket->extra += cur->extra + sizeof(bucket_t);
            delete_bucket(list, cur);
            add_bucket(list, bucket);
            return;
        }
    }
    add_bucket(list, bucket);
}

bucket_t* find_large_bucket(bucket_t** list, size_t size) {
    if (list == NULL) return NULL;
    bucket_t* cur = *list;
    while (cur != NULL) {
        if (cur->extra <= size) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

void destroy_large(bucket_t* bucket) {
    if (bucket == NULL) return;
    destroy_large(bucket->next);
    munmap((void*)bucket, bucket->extra + sizeof(bucket_t));
}