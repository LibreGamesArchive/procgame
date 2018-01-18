#ifndef HTABLE_INCLUDED
#define HTABLE_INCLUDED

#include <stdint.h>
#include "arr.h"

#ifndef HTABLE_KEY_LEN
#define HTABLE_KEY_LEN 32
#endif

#define HTABLE_ITER HTABLE_ENTRY
typedef struct { int bucket; int index; } HTABLE_ENTRY;

#define HTABLE_T(...) \
    struct { \
        ARR_T( struct { __VA_ARGS__ data; uint32_t hash; \
                        char key[HTABLE_KEY_LEN]; int key_len; })* buckets; \
        int nbuckets; }

#define HTABLE_INIT(ht, nb) \
    do { \
        (ht).nbuckets = nb; \
        (ht).buckets = malloc(sizeof(*(ht).buckets) * nb); \
        int i; \
        for(i = 0; i < nb; ++i) ARR_INIT((ht).buckets[i]); \
    } while(0)

#define HTABLE_DEINIT(ht) \
    do { \
        int i; \
        for(i = 0; i < (ht).nbuckets; ++i) ARR_DEINIT((ht).buckets[i]); \
        free((ht).buckets); \
    } while(0)

#define HTABLE_CLEAR(ht) \
    do { \
        int i; \
        for(i = 0; i < (ht).nbuckets; ++i) ARR_TRUNCATE((ht).buckets[i], 0); \
    } while(0)

#define HTABLE_HASH_(str, hash) \
    do { \
        hash = 5381; \
        const char* str_i = str; \
        while(*str_i) { \
            hash = ((hash << 5) + hash ) ^ (*str_i++); \
        } \
    } while(0)

#define HTABLE_SET(ht, k, ...) \
    do { \
        if(k == NULL || !k[0]) break; \
        uint32_t HT_HASH_; \
        HTABLE_HASH_(k, HT_HASH_); \
        int HT_KEYLEN_ = strnlen(k, 32); \
        int HT_BUCKET_ = HT_HASH_ % (ht).nbuckets; \
        int HT_IDX_; \
        for(HT_IDX_ = 0; HT_IDX_ < (ht).buckets[HT_BUCKET_].len; ++HT_IDX_) { \
            if((ht).buckets[HT_BUCKET_].data[HT_IDX_].hash == HT_HASH_ \
            && (ht).buckets[HT_BUCKET_].data[HT_IDX_].key_len == HT_KEYLEN_ \
            && strncmp((ht).buckets[HT_BUCKET_].data[HT_IDX_].key, k, HTABLE_KEY_LEN) == 0) { \
                break; \
            } \
        } \
        if(HT_IDX_ == (ht).buckets[HT_BUCKET_].cap) { \
            int new_size = (ht).buckets[HT_BUCKET_].cap * 1.5; \
            if(new_size == 0) new_size = 2; \
            ARR_RESERVE((ht).buckets[HT_BUCKET_], new_size); \
        } \
        strncpy((ht).buckets[HT_BUCKET_].data[HT_IDX_].key, k, HTABLE_KEY_LEN); \
        (ht).buckets[HT_BUCKET_].data[HT_IDX_].data = __VA_ARGS__; \
        (ht).buckets[HT_BUCKET_].data[HT_IDX_].hash = HT_HASH_; \
        (ht).buckets[HT_BUCKET_].data[HT_IDX_].key_len = HT_KEYLEN_; \
        (ht).buckets[HT_BUCKET_].len = HT_IDX_ + 1; \
    } while(0)

#define HTABLE_GET(ht, k, ptr) \
    do { \
        ptr = NULL; \
        uint32_t HT_HASH_; \
        HTABLE_HASH_(k, HT_HASH_); \
        int HT_KEYLEN_ = strnlen(k, 32); \
        int HT_BUCKET_ = HT_HASH_ % (ht).nbuckets; \
        int HT_IDX_ = 0; \
        for(HT_IDX_ = 0; HT_IDX_ < (ht).buckets[HT_BUCKET_].len; ++HT_IDX_) { \
            if((ht).buckets[HT_BUCKET_].data[HT_IDX_].hash == HT_HASH_ \
            && (ht).buckets[HT_BUCKET_].data[HT_IDX_].key_len == HT_KEYLEN_ \
            && strncmp((ht).buckets[HT_BUCKET_].data[HT_IDX_].key, k, HTABLE_KEY_LEN) == 0) { \
                ptr = &(ht).buckets[HT_BUCKET_].data[HT_IDX_].data; \
                break; \
            } \
        } \
    } while(0)

#define HTABLE_UNSET(ht, k) \
    do { \
        uint32_t HT_HASH_; \
        HTABLE_HASH_(k, HT_HASH_); \
        int HT_KEYLEN_ = strnlen(k, 32); \
        int HT_BUCKET_ = HT_HASH_ % (ht).nbuckets; \
        int HT_IDX_ = 0; \
        for(HT_IDX_ = 0; HT_IDX_ < (ht).buckets[HT_BUCKET_].len; ++HT_IDX_) { \
            if((ht).buckets[HT_BUCKET_].data[HT_IDX_].hash == HT_HASH_ \
            && (ht).buckets[HT_BUCKET_].data[HT_IDX_].key_len == HT_KEYLEN_ \
            && strncmp((ht).buckets[HT_BUCKET_].data[HT_IDX_].key, k, HT_KEYLEN_) == 0) { \
                ARR_SWAPSPLICE((ht).buckets[HT_BUCKET_], HT_IDX_, 1); \
                break; \
            } \
        } \
    } while(0)

#define HTABLE_MERGE(dst, src) \
    do { \
        int i, j; \
        for(i = 0; i < src.nbuckets; ++i) for(j = 0; j < src.buckets[i].len; ++j) { \
            int dst_bucket = src.buckets[i].data[j].hash % dst.nbuckets; \
            if(dst.buckets[dst_bucket].cap == dst.buckets[dst_bucket].len) { \
                int new_size = dst.buckets[dst_bucket].cap * 1.5; \
                if(new_size == 0) new_size = 2; \
                ARR_RESERVE(dst.buckets[dst_bucket], new_size); \
            } \
            ARR_PUSH(dst.buckets[dst_bucket], src.buckets[i].data[j]); \
        } \
    } while(0)

#define HTABLE_INVALID_ENTRY    HTABLE_NULL_ENTRY
#define HTABLE_NULL_ENTRY   ((HTABLE_ENTRY){ -1, 0 })
#define HTABLE_ENTRY_IS_NULL(entry) (HTABLE_EQUALS(entry, HTABLE_NULL_ENTRY))
#define HTABLE_GET_ENTRY(ht, k, entry) \
    do { \
        entry = (HTABLE_ENTRY){ -1, 0 }; \
        uint32_t HT_HASH_; \
        HTABLE_HASH_(k, HT_HASH_); \
        int HT_KEYLEN_ = strnlen(k, 32); \
        int HT_BUCKET_ = HT_HASH_ % (ht).nbuckets; \
        int HT_IDX_ = 0; \
        for(HT_IDX_ = 0; HT_IDX_ < (ht).buckets[HT_BUCKET_].len; ++HT_IDX_) { \
            if((ht).buckets[HT_BUCKET_].data[HT_IDX_].hash == HT_HASH_ \
            && (ht).buckets[HT_BUCKET_].data[HT_IDX_].key_len == HT_KEYLEN_ \
            && strncmp((ht).buckets[HT_BUCKET_].data[HT_IDX_].key, k, HTABLE_KEY_LEN) == 0) { \
                entry = (HTABLE_ENTRY){ HT_BUCKET_, HT_IDX_ }; \
                break; \
            } \
        } \
    } while(0)

#define HTABLE_ITER_BEGIN(ht, entry) \
    do { \
        entry = (HTABLE_ENTRY){ .index = -1, .bucket = 0 }; \
        HTABLE_ITER_ADVANCE(ht, entry); \
    } while(0)

#define HTABLE_ENTRY_VALID(ht, entry) \
    (entry.bucket >= 0 && entry.bucket < (ht).nbuckets \
    && entry.index >= 0 && entry.index < (ht).buckets[entry.bucket].len)

#define HTABLE_ENTRY_EQUALS(entry_a, entry_b) \
    (entry_a.index == entry_b.index && entry_a.bucket == entry_b.bucket)

#define HTABLE_ITER_END(ht, entry) (entry.bucket >= (ht).nbuckets)

#define HTABLE_ENTRY_ADVANCE HTABLE_ITER_ADVANCE
#define HTABLE_ITER_ADVANCE(ht, entry) \
    do { \
        while(!HTABLE_ITER_END(ht, entry)) { \
            if(++entry.index >= (ht).buckets[entry.bucket].len) { \
                entry.index = -1; ++entry.bucket; continue; \
            } else break; \
        } \
    } while(0)

#define HTABLE_ENTRY_NEXT HTABLE_ITER_NEXT
#define HTABLE_ITER_NEXT(ht, entry, k, ptr) \
    do { \
        HTABLE_ITER_UNPACK(ht, entry, k, ptr); \
        HTABLE_ITER_ADVANCE(ht, entry); \
    } while(0)

#define HTABLE_ITER_NEXT_PTR(ht, entry, ptr) \
    do { \
        HTABLE_ITER_PTR(ht, entry, ptr); \
        HTABLE_ITER_ADVANCE(ht, entry); \
    } while(0)

#define HTABLE_ITER_NEXT_KEY(ht, entry, k) \
    do { \
        HTABLE_ITER_KEY(ht, entry, k); \
        HTABLE_ITER_ADVANCE(ht, entry); \
    } while(0)

#define HTABLE_ENTRY_KEY HTABLE_ITER_KEY
#define HTABLE_ITER_KEY(ht, entry, k) \
    do{ \
        if(HTABLE_ENTRY_VALID(ht, entry)) { \
            k = (ht).buckets[entry.bucket].data[entry.index].key; \
        else k = NULL; \
    } while(0)

#define HTABLE_ENTRY_PTR HTABLE_ITER_PTR
#define HTABLE_ITER_PTR(ht, entry, ptr) \
    do{ \
        if(HTABLE_ENTRY_VALID(ht, entry)) \
            ptr = &(ht).buckets[entry.bucket].data[entry.index].data; \
        else ptr = NULL; \
    } while(0)

#define HTABLE_ENTRY_UNPACK HTABLE_ITER_UNPACK
#define HTABLE_ITER_UNPACK(ht, entry, k, ptr) \
    do{ \
        if(HTABLE_ENTRY_VALID(ht, entry)) { \
            k = (ht).buckets[entry.bucket].data[entry.index].key; \
            ptr = &(ht).buckets[entry.bucket].data[entry.index].data; \
        } else { k = NULL; ptr = NULL; } \
    } while(0)

#endif
