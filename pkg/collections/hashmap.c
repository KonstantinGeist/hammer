/* *****************************************************************************
*
*   Copyright (c) Konstantin Geist. All rights reserved.
*
*   The use and distribution terms for this software are contained in the file
*   named License.txt, which can be found in the root of this distribution.
*   By using this software in any fashion, you are agreeing to be bound by the
*   terms of this license.
*
*   You must not remove this notice, or any other, from this software.
*
* ******************************************************************************/

#include <collections/hashmap.h>
#include <core/allocator.h>
#include <core/hash.h>
#include <core/math.h>
#include <core/string.h>
#include <core/utils.h>

#define HM_HASHMAP_GROWTH_FACTOR 2

typedef struct _hmHashMapEntry {
    struct _hmHashMapEntry* next;
    char                    payload[1]; /* the size afterwards depends on key_size + value_size */
} hmHashMapEntry;

#define hmHashMapEntryGetKey(hashmap, entry) ((entry)->payload)
#define hmHashMapEntryGetValue(hashmap, entry) (((entry)->payload) + ((hashmap)->key_size))
static hm_nint hmHashMapGetBucketIndex(hmHashMap* hash_map, void* key);
static hm_bool hmHashMapAreKeysEqual(hmHashMap* hash_map, void* value1, void* value2);
static hmHashMapEntry* hmHashMapEntryFindByBucketIndexAndKey(hmHashMap* hash_map, hm_nint bucket_index, void* key);
static hmHashMapEntry* hmHashMapEntryFindByKey(hmHashMap* hash_map, void* key);
static hmError hmHashMapRehash(hmHashMap* hash_map);

hmError hmCreateHashMap(
    struct _hmAllocator* allocator,
    hmHashMapHashFunc    hash_func,
    hmHashMapEqualsFunc  equals_func,
    hmDisposeFunc        key_dispose_func,
    hmDisposeFunc        value_dispose_func,
    hm_nint              key_size,
    hm_nint              value_size,
    hm_nint              initial_capacity,
    hm_float64           load_factor,
    hm_uint32            hash_salt,
    hmHashMap*           in_hashmap
)
{
    if (!initial_capacity || load_factor < 0.5 || load_factor > 1.0) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hm_nint buckets_size = 0;
    HM_TRY(hmMulNint(sizeof(hmHashMapEntry*), initial_capacity, &buckets_size));
    in_hashmap->buckets = hmAllocZeroInitialized(allocator, buckets_size);
    if (!in_hashmap->buckets) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    in_hashmap->allocator = allocator;
    in_hashmap->hash_func = hash_func;
    in_hashmap->equals_func = equals_func;
    in_hashmap->key_dispose_func = key_dispose_func;
    in_hashmap->value_dispose_func = value_dispose_func;
    in_hashmap->key_size = key_size;
    in_hashmap->value_size = value_size;
    in_hashmap->count = 0;
    in_hashmap->bucket_count = initial_capacity;
    /* No safe math operations here, because load_factor is in the range [0.5, 1.0]. */
    in_hashmap->threshold = (hm_nint)(initial_capacity * load_factor);
    in_hashmap->load_factor = load_factor;
    in_hashmap->hash_salt = hash_salt;
    return HM_OK;
}

hmError hmCreateHashMapWithStringKeys(
    struct _hmAllocator* allocator,
    hmDisposeFunc        value_dispose_func,
    hm_nint              value_size,
    hm_nint              initial_capacity,
    hm_float64           load_factor,
    hm_uint32            hash_salt,
    hmHashMap*           in_hashmap
)
{
    return hmCreateHashMap(
        allocator,
        &hmStringHashFunc,
        &hmStringEqualsFunc,
        &hmStringDisposeFunc, /* key_dispose_func */
        value_dispose_func,
        sizeof(hmString),
        value_size,
        initial_capacity,
        load_factor,
        hash_salt,
        in_hashmap
    );
}

hmError hmCreateHashMapWithStringRefKeys(
    struct _hmAllocator* allocator,
    hmDisposeFunc        value_dispose_func,
    hm_nint              value_size,
    hm_nint              initial_capacity,
    hm_float64           load_factor,
    hm_uint32            hash_salt,
    hmHashMap*           in_hashmap
)
{
    return hmCreateHashMap(
        allocator,
        &hmStringRefHashFunc,
        &hmStringRefEqualsFunc,
        HM_NULL,
        value_dispose_func,
        sizeof(hmString*),
        value_size,
        initial_capacity,
        load_factor,
        hash_salt,
        in_hashmap
    );
}

hmError hmHashMapDispose(hmHashMap* hash_map)
{
    hmError err = HM_OK;
    for (hm_nint i = 0; i < hash_map->bucket_count; i++) {
        hmHashMapEntry* entry = hash_map->buckets[i];
        hmHashMapEntry* next_entry = HM_NULL;
        while (entry) {
            if (hash_map->key_dispose_func) {
                void* key = hmHashMapEntryGetKey(hash_map, entry);
                err = hmMergeErrors(err, hash_map->key_dispose_func(key));
            }
            if (hash_map->value_dispose_func) {
                void* value = hmHashMapEntryGetValue(hash_map, entry);
                err = hmMergeErrors(err, hash_map->value_dispose_func(value));
            }
            next_entry = entry->next;
            hmFree(hash_map->allocator, entry);
            entry = next_entry;
        }
    }
    hmFree(hash_map->allocator, hash_map->buckets);
    return err;
}

hmError hmHashMapPut(hmHashMap* hash_map, void* key, void* value)
{
    if (hash_map->count > hash_map->threshold) {
        HM_TRY(hmHashMapRehash(hash_map));
    }
    hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, key);
    hmHashMapEntry* entry = hmHashMapEntryFindByBucketIndexAndKey(hash_map, bucket_index, key);
    if (entry) {
        void* value_dest = hmHashMapEntryGetValue(hash_map, entry);
        if (hash_map->value_dispose_func) {
            HM_TRY(hash_map->value_dispose_func(value_dest));
        }
        hmCopyMemory(value_dest, value, hash_map->value_size);
        return HM_OK;
    }
    hm_nint new_count = 0;
    HM_TRY(hmAddNint(hash_map->count, 1, &new_count));
    hm_nint entry_size = 0;
    HM_TRY(hmAddNint3(sizeof(hmHashMapEntry) - 1, hash_map->key_size, hash_map->value_size, &entry_size)); /* -1 for "char[1] payload" */
    hmHashMapEntry* new_entry = hmAlloc(
        hash_map->allocator,
        entry_size
    );
    if (!new_entry) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    void* key_dest = hmHashMapEntryGetKey(hash_map, new_entry);
    void* value_dest = hmHashMapEntryGetValue(hash_map, new_entry);
    hmCopyMemory(key_dest, key, hash_map->key_size);
    hmCopyMemory(value_dest, value, hash_map->value_size);
    new_entry->next = hash_map->buckets[bucket_index];
    hash_map->buckets[bucket_index] = new_entry;
    hash_map->count = new_count;
    return HM_OK;
}

hmError hmHashMapGet(hmHashMap* hash_map, void* key, void* in_value)
{
    hmHashMapEntry* entry = hmHashMapEntryFindByKey(hash_map, key);
    if (!entry) {
        return HM_ERROR_NOT_FOUND;
    }
    void* value_src = hmHashMapEntryGetValue(hash_map, entry);
    hmCopyMemory(in_value, value_src, hash_map->value_size);
    return HM_OK;
}

hmError hmHashMapGetRef(hmHashMap* hash_map, void* key, void** in_value)
{
    hmHashMapEntry* entry = hmHashMapEntryFindByKey(hash_map, key);
    if (!entry) {
        return HM_ERROR_NOT_FOUND;
    }
    *in_value = hmHashMapEntryGetValue(hash_map, entry);
    return HM_OK;
}

hm_bool hmHashMapContains(hmHashMap* hash_map, void* key)
{
    hmHashMapEntry* entry = hmHashMapEntryFindByKey(hash_map, key);
    return entry != HM_NULL;
}

hmError hmHashMapRemove(hmHashMap* hash_map, void* key, hm_bool* out_removed)
{
    hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, key);
    hmHashMapEntry* entry = hash_map->buckets[bucket_index];
    hmHashMapEntry* prev_entry = HM_NULL;
    while (entry) {
        void* key_candidate = hmHashMapEntryGetKey(hash_map, entry);
        if (hmHashMapAreKeysEqual(hash_map, key, key_candidate)) {
            if (hash_map->key_dispose_func) {
                HM_TRY(hash_map->key_dispose_func(key_candidate));
            }
            if (hash_map->value_dispose_func) {
                void* value = hmHashMapEntryGetValue(hash_map, entry);
                HM_TRY(hash_map->value_dispose_func(value));
            }
            if (prev_entry) {
                prev_entry->next = entry->next;
            } else {
                hash_map->buckets[bucket_index] = entry->next;
            }
            hmFree(hash_map->allocator, entry);
            hash_map->count--;
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

static hm_nint hmHashMapGetBucketIndex(hmHashMap* hash_map, void* key)
{
    hm_uint32 hash;
    if (hash_map->hash_func) {
        hash = hash_map->hash_func(key, hash_map->hash_salt);
    } else {
        hash = hmHash(key, hash_map->key_size, hash_map->hash_salt);
    }
    return hash % hash_map->bucket_count;
}

static hm_bool hmHashMapAreKeysEqual(hmHashMap* hash_map, void* value1, void* value2)
{
    if (hash_map->equals_func) {
        return hash_map->equals_func(value1, value2);
    }
    return hmCompareMemory(value1, value2, hash_map->key_size) == 0;
}

static hmHashMapEntry* hmHashMapEntryFindByBucketIndexAndKey(hmHashMap* hash_map, hm_nint bucket_index, void* key)
{
    hmHashMapEntry* entry = hash_map->buckets[bucket_index];
    while (entry) {
        void* key_candidate = hmHashMapEntryGetKey(hash_map, entry);
        if (hmHashMapAreKeysEqual(hash_map, key, key_candidate)) {
            return entry;
        }
        entry = entry->next;
    }
    return HM_NULL;
}

static hmHashMapEntry* hmHashMapEntryFindByKey(hmHashMap* hash_map, void* key)
{
    hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, key);
    return hmHashMapEntryFindByBucketIndexAndKey(hash_map, bucket_index, key);
}

static hmError hmHashMapRehash(hmHashMap* hash_map)
{
    hmHashMapEntry** old_buckets = hash_map->buckets;
    hm_nint old_bucket_count = hash_map->bucket_count;
    hm_nint new_bucket_count = 0;
    HM_TRY(hmMulNint(old_bucket_count, HM_HASHMAP_GROWTH_FACTOR, &new_bucket_count));
    hm_nint new_buckets_size = 0;
    HM_TRY(hmMulNint(sizeof(hmHashMapEntry*), new_bucket_count, &new_buckets_size));
    hmHashMapEntry** new_buckets = hmAllocZeroInitialized(hash_map->allocator, new_buckets_size);
    if (!new_buckets) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hash_map->buckets = new_buckets;
    hash_map->bucket_count = new_bucket_count;
    hash_map->threshold = (int)(new_bucket_count * hash_map->load_factor); /* no safe math operations because load_factor is in the range [0.5, 1.0] */
    for (hm_nint i = 0; i < old_bucket_count; i++) {
        hmHashMapEntry* old_entry = old_buckets[i];
        while (old_entry) {
            void* key = hmHashMapEntryGetKey(hash_map, old_entry);
            hm_nint new_bucket_index = hmHashMapGetBucketIndex(hash_map, key);
            hmHashMapEntry* new_entry = new_buckets[new_bucket_index];
            if (new_entry) {
                while (new_entry->next) {
                    new_entry = new_entry->next;
                }
                new_entry->next = old_entry;
            } else {
                new_buckets[new_bucket_index] = old_entry;
            }
            hmHashMapEntry* next_old_entry = old_entry->next;
            old_entry->next = HM_NULL;
            old_entry = next_old_entry;
        }
    }
    hmFree(hash_map->allocator, old_buckets);
    return HM_OK;
}
