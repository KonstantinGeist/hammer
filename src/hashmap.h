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

#ifndef HM_HASHMAP_H
#define HM_HASHMAP_H

#include "common.h"
#include "array.h"

#define HM_DEFAULT_HASHMAP_CAPACITY 17
#define HM_DEFAULT_HASHMAP_LOAD_FACTOR 0.75

struct _hmAllocator;
struct _hmHashMapEntry;

typedef int(*hmHashMapHashFunc)(void* key);
typedef hm_bool(*hmHashMapEqualsFunc)(void* value1, void* value2);

typedef struct {
    struct _hmAllocator*       allocator;
    struct _hmHashMapEntry**   buckets;  /* A list of buckets which contain linked lists of entries of size key_size+value_size. */
    hmHashMapHashFunc          hash_func;
    hmHashMapEqualsFunc        equals_func;
    hmDisposeFunc              dispose_func; // can be HM_NULL
    hm_nint                    key_size;
    hm_nint                    value_size;
    hm_nint                    count;
    hm_nint                    bucket_count;
    hm_float                   threshold;
    hm_float                   load_factor;
} hmHashMap;

/* Creates a hash map, with provided hash_func, equals_func, key/value sizes.
   Load factor should be between 0.5 and 1.0 (preferred value is HM_DEFAULT_HASHMAP_LOAD_FACTOR).
   Initial capacity can be set to HM_DEFAULT_HASHMAP_CAPACITY. */
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
);
/* Puts a value in the map by the given key. Note that if dispose_func is provided, it's always called on the old value,
   even if it's the same value -- which can lead to use-after-free if used without care. */
hmError hmHashMapPut(hmHashMap* hash_map, void* key, void* value);
/* Tries to retrieve an element from the map. Returns HM_ERROR_NOT_FOUND if no element by the given key is found. */
hmError hmHashMapGet(hmHashMap* hash_map, void* key, void* in_value);
/* Removes an item from the map, by the given key. Returns out_removed, if the element was actually removed.
   out_removed can be HM_NULL. */
hmError hmHashMapRemove(hmHashMap* hash_map, void* key, hm_bool* out_removed);
hmError hmHashMapDispose(hmHashMap* hash_map);

#endif /* HM_HASHMAP_H */
