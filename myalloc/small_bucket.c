#include "small_bucket.h"
bucket_t* create_small() {
    return create_bucket(getpagesize() - sizeof(bucket_t));
}

void free_small(bucket_t** list, void* ptr) {
    if ((list == NULL) || (ptr == NULL)) return;
    bucket_t* cur_bucket = *list;
    while (cur_bucket != NULL) {
        void* mem = cur_bucket->mem;
        if (mem <= ptr && ptr < mem + getpagesize() - sizeof(bucket_t)) {
            cur_bucket->extra ^= 1 << (ptr - mem) / max_small_size();
            break;
        }
    }
    if ((cur_bucket != NULL) && (cur_bucket->extra == 0)) {
        delete_small_bucket(list, cur_bucket);
    }
}

void destroy_small(bucket_t* bucket) {
    if (bucket == NULL) return;
    destroy_small(bucket->next);
    munmap((void*)bucket, getpagesize());
}

bucket_t* find_small_bucket(bucket_t** list) {
    if (list == NULL) return;
    bucket_t* cur = *list;
    size_t mask = (size_t) - 1;
    while (cur != NULL) {
        if ((cur->extra & mask) != mask) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}


size_t max_small_size() {
    return (getpagesize() - sizeof(bucket_t)) / (sizeof(size_t) * 8);
}