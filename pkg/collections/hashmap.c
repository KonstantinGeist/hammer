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

typedef struct hmHashMapEntry_ {
    struct hmHashMapEntry_* next;
    char                    payload[1]; /* the size afterwards depends on key_size + value_size */
} hmHashMapEntry;

#define hmHashMapEntryGetKey(hashmap, entry) ((entry)->payload)
#define hmHashMapEntryGetValue(hashmap, entry) (((entry)->payload) + ((hashmap)->key_size))
static hm_nint hmHashMapGetBucketIndex(hmHashMap* hash_map, void* key);
static hm_bool hmHashMapAreKeysEqual(hmHashMap* hash_map, void* value1, void* value2);
static hmHashMapEntry* hmHashMapEntryFindByBucketIndexAndKey(hmHashMap* hash_map, hm_nint bucket_index, void* key);
static hmHashMapEntry* hmHashMapEntryFindByKey(hmHashMap* hash_map, void* key);
static hmError hmHashMapRehash(hmHashMap* hash_map);
static hmError hmHashMapRemoveImpl(hmHashMap* hash_map, void* key, hm_bool* out_removed, hm_bool use_dispose_funcs);

hmError hmCreateHashMap(
    hmAllocator*        allocator,
    hmHashMapHashFunc   hash_func_opt,
    hmHashMapEqualsFunc equals_func_opt,
    hmDisposeFunc       key_dispose_func_opt,
    hmDisposeFunc       value_dispose_func_opt,
    hm_nint             key_size,
    hm_nint             value_size,
    hm_nint             initial_capacity,
    hm_float64          load_factor,
    hm_uint32           hash_salt,
    hmHashMap*          in_hashmap
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
    in_hashmap->hash_func_opt = hash_func_opt;
    in_hashmap->equals_func_opt = equals_func_opt;
    in_hashmap->key_dispose_func_opt = key_dispose_func_opt;
    in_hashmap->value_dispose_func_opt = value_dispose_func_opt;
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
    hmAllocator*  allocator,
    hmDisposeFunc value_dispose_func_opt,
    hm_nint       value_size,
    hm_nint       initial_capacity,
    hm_float64    load_factor,
    hm_uint32     hash_salt,
    hmHashMap*    in_hashmap
)
{
    return hmCreateHashMap(
        allocator,
        &hmStringHashFunc,
        &hmStringEqualsFunc,
        &hmStringDisposeFunc, /* key_dispose_func_opt */
        value_dispose_func_opt,
        sizeof(hmString),
        value_size,
        initial_capacity,
        load_factor,
        hash_salt,
        in_hashmap
    );
}

hmError hmCreateHashMapWithStringRefKeys(
    hmAllocator*  allocator,
    hmDisposeFunc value_dispose_func_opt,
    hm_nint       value_size,
    hm_nint       initial_capacity,
    hm_float64    load_factor,
    hm_uint32     hash_salt,
    hmHashMap*    in_hashmap
)
{
    return hmCreateHashMap(
        allocator,
        &hmStringRefHashFunc,
        &hmStringRefEqualsFunc,
        HM_NULL,
        value_dispose_func_opt,
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
            if (hash_map->key_dispose_func_opt) {
                void* key = hmHashMapEntryGetKey(hash_map, entry);
                err = hmMergeErrors(err, hash_map->key_dispose_func_opt(key));
            }
            if (hash_map->value_dispose_func_opt) {
                void* value = hmHashMapEntryGetValue(hash_map, entry);
                err = hmMergeErrors(err, hash_map->value_dispose_func_opt(value));
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
        if (hash_map->value_dispose_func_opt) {
            HM_TRY(hash_map->value_dispose_func_opt(value_dest));
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
    return hmHashMapRemoveImpl(hash_map, key, out_removed, HM_TRUE); /* use_dispose_funcs = HM_TRUE */
}

hmError hmHashMapEnumerate(hmHashMap* hash_map, hmHashMapEnumerateFunc enumerate_func, void* user_data)
{
    for (hm_nint i = 0; i < hash_map->bucket_count; i++) {
        hmHashMapEntry* bucket = hash_map->buckets[i];
        for (hmHashMapEntry* entry = bucket; entry; entry = entry->next) {
            void* key = hmHashMapEntryGetKey(hash_map, entry);
            void* value = hmHashMapEntryGetValue(hash_map, entry);
            HM_TRY(enumerate_func(key, value, user_data));
        }
    }
    return HM_OK;
}

hmError hmHashMapMoveTo(hmHashMap* hash_map, hmHashMap* in_dest_hash_map)
{
    hmError err = HM_OK;
    /* Validation (keys in the dest hashmap should not conflict with the keys in the source hashmap). */
    for (hm_nint i = 0; i < hash_map->bucket_count; i++) {
        hmHashMapEntry* bucket = hash_map->buckets[i];
        for (hmHashMapEntry* entry = bucket; entry; entry = entry->next) {
            void* key = hmHashMapEntryGetKey(hash_map, entry);
            hm_bool key_exists = hmHashMapContains(in_dest_hash_map, key);
            if (key_exists) {
                return HM_ERROR_INVALID_ARGUMENT;
            }
        }
    }
    /* Moving. */
    for (hm_nint i = 0; i < hash_map->bucket_count; i++) {
        hmHashMapEntry* bucket = hash_map->buckets[i];
        for (hmHashMapEntry* entry = bucket; entry; entry = entry->next) {
            void* key = hmHashMapEntryGetKey(hash_map, entry);
            void* value = hmHashMapEntryGetValue(hash_map, entry);
            HM_TRY_OR_FINALIZE(err, hmHashMapPut(in_dest_hash_map, key, value));
        }
    }
HM_ON_FINALIZE
    /* Rollback/cleanup. */
    hmHashMap *hash_map_to_iterate = HM_NULL,
              *hash_map_to_clear = HM_NULL;
    if (err == HM_OK) {
        /* On success, we want to remove all copied items from the old map. */
        hash_map_to_iterate = in_dest_hash_map;
        hash_map_to_clear = hash_map;
    } else {
        /* On error, we want to undo everything we've done so far. So we iterate over the old hashmap again and remove everything
           which has been added to the dest map. */
        hash_map_to_iterate = hash_map;
        hash_map_to_clear = in_dest_hash_map;
    }
    for (hm_nint i = 0; i < hash_map_to_iterate->bucket_count; i++) {
        hmHashMapEntry* bucket = hash_map_to_iterate->buckets[i];
        for (hmHashMapEntry* entry = bucket; entry; entry = entry->next) {
            void* key = hmHashMapEntryGetKey(hash_map_to_iterate, entry);
            /* use_dispose_funcs = HM_FALSE, because we don't the dispose functions to be called, as the values are still alive, just moved */
            err = hmMergeErrors(err, hmHashMapRemoveImpl(hash_map_to_clear, key, HM_NULL, HM_FALSE));
        }
    }
    return err;
}

static hm_nint hmHashMapGetBucketIndex(hmHashMap* hash_map, void* key)
{
    hm_uint32 hash;
    if (hash_map->hash_func_opt) {
        hash = hash_map->hash_func_opt(key, hash_map->hash_salt);
    } else {
        hash = hmHash(key, hash_map->key_size, hash_map->hash_salt);
    }
    return hash % hash_map->bucket_count;
}

static hm_bool hmHashMapAreKeysEqual(hmHashMap* hash_map, void* value1, void* value2)
{
    if (hash_map->equals_func_opt) {
        return hash_map->equals_func_opt(value1, value2);
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

static hmError hmHashMapRemoveImpl(hmHashMap* hash_map, void* key, hm_bool* out_removed, hm_bool use_dispose_funcs)
{
    hm_nint bucket_index = hmHashMapGetBucketIndex(hash_map, key);
    hmHashMapEntry* entry = hash_map->buckets[bucket_index];
    hmHashMapEntry* prev_entry = HM_NULL;
    while (entry) {
        void* key_candidate = hmHashMapEntryGetKey(hash_map, entry);
        if (hmHashMapAreKeysEqual(hash_map, key, key_candidate)) {
            if (use_dispose_funcs && hash_map->key_dispose_func_opt) {
                HM_TRY(hash_map->key_dispose_func_opt(key_candidate));
            }
            if (use_dispose_funcs && hash_map->value_dispose_func_opt) {
                void* value = hmHashMapEntryGetValue(hash_map, entry);
                HM_TRY(hash_map->value_dispose_func_opt(value));
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
