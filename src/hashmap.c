// *****************************************************************************
//
//  Copyright (c) Konstantin Geist. All rights reserved.
//
//  The use and distribution terms for this software are contained in the file
//  named License.txt, which can be found in the root of this distribution.
//  By using this software in any fashion, you are agreeing to be bound by the
//  terms of this license.
//
//  You must not remove this notice, or any other, from this software.
//
// *****************************************************************************

#include "hashmap.h"
#include "allocator.h"
#include "utils.h"

typedef struct _hmHashMapEntry {
    struct _hmHashMapEntry* next;
    char                    payload[1]; // the size afterwards depends on key_size+value_size
} hmHashMapEntry;

#define hmHashMapEntryGetKey(hashmap, entry) (&((entry)->payload[0]))
#define hmHashMapEntryGetValue(hashmap, entry) (&((entry)->payload[0])+((hashmap)->key_size))

static hm_nint hmHashMapGetBucketIndex(hmHashMap* hash_map, void* key)
{
    int hash = hash_map->hash_func(key);
    return abs(hash % hash_map->bucket_count);
}

static hmHashMapEntry* hmHashMapEntryFindByKey(hmHashMap* hash_map, void* key)
{
    hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, key);
    hmHashMapEntry* entry = hash_map->buckets[bucket_index];
    while (entry) {
        if (hash_map->equals_func(key, hmHashMapEntryGetKey(hash_map, entry))) {
            return entry;
        }
        entry = entry->next;
    }
    return HM_NULL;
}

hmError hmCreateHashMap(
    struct _hmAllocator* allocator,
    hmHashMapHashFunc    hash_func,
    hmHashMapEqualsFunc  equals_func,
    hmDisposeFunc        dispose_func,
    hm_nint              key_size,
    hm_nint              value_size,
    hm_nint              initial_capacity,
    hm_float             load_factor,
    hmHashMap*           in_hashmap
)
{
    if (!hash_func || !equals_func || !initial_capacity || load_factor <= 0.5 || load_factor > 1.0) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    key_size = hmAlignSize(key_size);
    value_size = hmAlignSize(value_size);
    in_hashmap->buckets = hmAllocZeroInitialized(allocator, sizeof(hmHashMapEntry*)*initial_capacity);
    if (!in_hashmap->buckets) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    in_hashmap->allocator = allocator;
    in_hashmap->hash_func = hash_func;
    in_hashmap->equals_func = equals_func;
    in_hashmap->dispose_func = dispose_func;
    in_hashmap->key_size = key_size;
    in_hashmap->value_size = value_size;
    in_hashmap->count = 0;
    in_hashmap->bucket_count = initial_capacity;
    in_hashmap->threshold = (hm_nint)(initial_capacity * load_factor);
    in_hashmap->load_factor = load_factor;
    return HM_OK;
}

static hmError hmHashMapRehash(hmHashMap* hash_map)
{
    hmHashMapEntry** old_buckets = hash_map->buckets;
    hm_nint old_bucket_count = hash_map->bucket_count;
    hm_nint new_capacity = old_bucket_count*2+1;
    hash_map->threshold = (int)(new_capacity * hash_map->load_factor);
    hmHashMapEntry** new_buckets = hmAllocZeroInitialized(hash_map->allocator, sizeof(hmHashMapEntry*)*new_capacity);
    if (!new_buckets) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hash_map->buckets = new_buckets;
    for (hm_nint i = 0; i < old_bucket_count; i++) {
        hmHashMapEntry* entry = old_buckets[i];
        while (entry) {
            void* key = hmHashMapEntryGetKey(hash_map, entry);
            hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, entry);
            hmHashMapEntry* entry_dest = hash_map->buckets[bucket_index];
            if (entry_dest) {
                while (entry_dest->next) {
                    entry_dest = entry_dest->next;
                }
                entry_dest->next = entry;
            } else {
                hash_map->buckets[bucket_index] = entry;
            }
            hmHashMapEntry* next = entry->next;
            entry->next = HM_NULL;
            entry = next;
        }
    }
    hmFree(hash_map->allocator, old_buckets);
    return HM_OK;
}

hmError hmHashMapPut(hmHashMap* hash_map, void* key, void* value)
{
    hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, key);
    hmHashMapEntry* entry = hmHashMapEntryFindByKey(hash_map, key);
    if (entry) {
        void* value_dest = hmHashMapEntryGetValue(hash_map, entry);
        if (hash_map->dispose_func) {
            hmError err = hash_map->dispose_func(value_dest);
            if (err != HM_OK) {
                return err;
            }
        }
        memcpy(value_dest, value, hash_map->value_size);
        return HM_OK;
    }
    if (hash_map->count > hash_map->threshold) {
        hmError err = hmHashMapRehash(hash_map);
        if (err != HM_OK) {
            return err;
        }
    }
    hmHashMapEntry* new_entry = hmAlloc(
        hash_map->allocator,
        sizeof(hmHashMapEntry)-1+hash_map->key_size+hash_map->value_size // -1 for "char[1] payload"
    );
    if (!new_entry) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    void* key_dest = hmHashMapEntryGetKey(hash_map, new_entry);
    void* value_dest = hmHashMapEntryGetValue(hash_map, new_entry);
    memcpy(key_dest, key, hash_map->key_size);
    memcpy(value_dest, value, hash_map->value_size);
    new_entry->next = hash_map->buckets[bucket_index];
    hash_map->buckets[bucket_index] = new_entry;
    hash_map->count++;
    return HM_OK;
}

hmError hmHashMapGet(hmHashMap* hash_map, void* key, void* in_value)
{
    hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, key);
    hmHashMapEntry* entry = hmHashMapEntryFindByKey(hash_map, key);
    if (entry) {
        void* value_src = &entry->payload[0]+hash_map->key_size;
        memcpy(in_value, value_src, hash_map->value_size);
        return HM_OK;
    }
    return HM_ERROR_NOT_FOUND;
}

hmError hmHashMapRemove(hmHashMap* hash_map, void* key, hm_bool* out_removed)
{
    hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, key);
    hmHashMapEntry* entry = hash_map->buckets[bucket_index];
    hmHashMapEntry* prev_entry = HM_NULL;
    while (entry) {
        void* key_candidate = hmHashMapEntryGetKey(hash_map, entry);
        if (hash_map->equals_func(key, key_candidate)) {
            if (hash_map->dispose_func) {
                void* value = hmHashMapEntryGetValue(hash_map, entry);
                hmError err = hash_map->dispose_func(value);
                if (err != HM_OK) {
                    return err;
                }
            }
            if (prev_entry == HM_NULL) {
                hash_map->buckets[bucket_index] = entry->next;
            } else {
                prev_entry->next = entry->next;
            }
            hmFree(hash_map->allocator, entry);
            hash_map->count++;
            if (out_removed) {
                *out_removed = HM_TRUE;
            }
            return HM_OK;
        }
        prev_entry = entry;
        entry = entry->next;
    }
    if (out_removed) {
        *out_removed = HM_FALSE;
    }
    return HM_OK;
}

hmError hmHashMapDispose(hmHashMap* hash_map)
{
    for (hm_nint i = 0; i < hash_map->bucket_count; i++) {
        hmHashMapEntry* entry = hash_map->buckets[i];
        hmHashMapEntry* next_entry = HM_NULL;
        while (entry) {
            next_entry = entry->next;
            hmFree(hash_map->allocator, entry);
            entry = next_entry;
        }
    }
    hmFree(hash_map->allocator, hash_map->buckets);
    return HM_OK;
}
