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

#ifndef HM_HASHMAP_H
#define HM_HASHMAP_H

#include <core/common.h>
#include <core/allocator.h>
#include <collections/array.h>

#define HM_HASHMAP_DEFAULT_CAPACITY 16
#define HM_HASHMAP_DEFAULT_LOAD_FACTOR 0.75

struct hmHashMapEntry_;

typedef hm_uint32 (*hmHashMapHashFunc)(void* key, hm_uint32 salt);
typedef hm_bool (*hmHashMapEqualsFunc)(void* value1, void* value2);
typedef hmError (*hmHashMapEnumerateFunc)(void* key, void* value, void* user_data);

typedef struct {
    hmAllocator*             allocator;
    struct hmHashMapEntry_** buckets;                /* A list of buckets which contain linked lists of entries of
                                                        size key_size + value_size. */
    hmHashMapHashFunc        hash_func_opt;          /* Optional hash function (can be HM_NULL) to locate items in the
                                                        bucket list. If it's not provided, uses hmHash(..) */
    hmHashMapEqualsFunc      equals_func_opt;        /* Optional equality function to locate items in the bucket list.
                                                        If it's not provided, bitwise comparisons are used, which are
                                                        not safe (see hmCreateHashMap(..)) */
    hmDisposeFunc            key_dispose_func_opt;   /* Optional key disposal function (can be HM_NULL). */
    hmDisposeFunc            value_dispose_func_opt; /* Optional value disposal function (can be HM_NULL). */
    hm_nint                  key_size;               /* Key size is specified upfront to make sure internal backing lists
                                                        for keys are of appropriate size. */
    hm_nint                  value_size;             /* Value size is specified upfront to make sure internal backing lists
                                                        for values are of appropriate size. */
    hm_nint                  count;                  /* Keeps track of the hashmap's size. */
    hm_nint                  bucket_count;           /* Specifies how many buckets there are in the `buckets` list. */
    hm_float64               threshold;              /* Specifies a threshold when the hashmap must be rebalanced (rehashed)
                                                        (see hmHashMapPut(..), for example). */
    hm_float64               load_factor;            /* Load factor is used to create new threshold values, in the range from [0.5, 1.0];
                                                        see hmCreateHashMap(..) */
    hm_uint32                hash_salt;              /* `hash_salt` is used to salt hashes to prevent against hash DoS attacks,
                                                        see hmHash(..) for more details. */
} hmHashMap;

/* Creates a hashmap, with provided hash_func, equals_func, key/value sizes.
   `hash_func` and `equals_func` are optional hash and equality functions to quickly locate items in the hashmap.
   If they are HM_NULL, in that case, bitwise comparisons are made. Built-in objects such as hmString already have
   their own hash functions, for example, hmStringHashFunc(..) and hmStringEqualsFunc(..)
   `key_dispose_func_opt` and `value_dispose_func_opt` are optional functions which specify how to dispose of keys/values
    when the hashmap itself is disposed. The values can be HM_NULL (nothing will be disposed in that case). If the functions
    are actually provided, then it's assumed that the keys/values passed to the hashmap become "owned" by the map ("moved").
   `initial_capacity` specifies the initial size of the internal backing lists; set it to a large value if the hashmap
    will be populated with many items. The value can be set to HM_HASHMAP_DEFAULT_CAPACITY. Returns HM_ERROR_INVALID_ARGUMENT if it's zero.
   `load_factor` tells when to rebalance (rehash) the hashmap, in the range from [0.5, 1.0]. Preferred value is HM_HASHMAP_DEFAULT_LOAD_FACTOR.
   `hash_salt` is used to salt hashes to prevent against hash DoS attacks, see hmHash(..) for more details.
   WARNING The default, bitwise comparison-based hashing is unsafe with structs because the compiler can
   add padding with uninitialized garbage. */
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
);
/* A helper function over hmCreateHashMap to create a hashmap whose keys are strings (one of the most common cases). */
hmError hmCreateHashMapWithStringKeys(
    hmAllocator*  allocator,
    hmDisposeFunc value_dispose_func_opt,
    hm_nint       value_size,
    hm_nint       initial_capacity,
    hm_float64    load_factor,
    hm_uint32     hash_salt,
    hmHashMap*    in_hashmap
);
/* A helper function over hmCreateHashMap to create a hashmap whose keys are string references (one of the most common cases). */
hmError hmCreateHashMapWithStringRefKeys(
    hmAllocator*  allocator,
    hmDisposeFunc value_dispose_func_opt,
    hm_nint       value_size,
    hm_nint       initial_capacity,
    hm_float64    load_factor,
    hm_uint32     hash_salt,
    hmHashMap*    in_hashmap
);
hmError hmHashMapDispose(hmHashMap* hash_map);
/* Puts a value in the map by the given key. Note that if `key_dispose_func_opt` or `value_dispose_func_opt` are provided,
   it's always called on the old value, even if it's the same value -- which can lead to use-after-free if used without care.
   `key` and `value` are pointers to values, not the values themselves. If `key_dispose_func_opt` or `value_dispose_func_opt`
   are provided, it's assumed that the hashmap takes ownership of the keys/values. */
hmError hmHashMapPut(hmHashMap* hash_map, void* key, void* value);
/* Tries to retrieve an item from the map. Returns HM_ERROR_NOT_FOUND if no item by the given key is found.
   Never retains the key value (safe to use with views). The returned value is copied by value; that is, it's not safe
   to delete such an object if its ownership belongs to the hashmap. Use hmHashMapGetRef if you need a reference. */
hmError hmHashMapGet(hmHashMap* hash_map, void* key, void* in_value);
/* Same as hmHashMapGet, except returns a pointer to the value directly as stored in the hashmap, instead of copying it by value.
   The value is stable after rehashing. However, the reference will point to a different object if a different value is
   put in the map with the same key. The reference is invalidated when it's removed from the map.
   Useful in optimizations, as it avoids copying/moving the object, which can be also updated in-place. */
hmError hmHashMapGetRef(hmHashMap* hash_map, void* key, void** in_value);
/* Returns true if an item by the given key is found in the hashmap. Similar to checking hmHashMapGet(..) for HM_ERROR_NOT_FOUND,
   except it's more performant and has a simpler interface. */
hm_bool hmHashMapContains(hmHashMap* hash_map, void* key);
/* Removes an item from the map, by the given key. Returns `out_removed_opt`, if the item was actually removed.
   `out_removed` can be HM_NULL. */
hmError hmHashMapRemove(hmHashMap* hash_map, void* key, hm_bool* out_removed_opt);
/* Enumerates all the keys and values in the map by calling function `enumerate_func`. The error returned from `enumerate_func`
   is returned from hmHashMapEnumerate(..) as is. On any error other than HM_OK, enumeration is immediately terminated.
   Use `user_data` to pass additional context to the callback.
   Iteration order is not guaranteed.
   WARNING Modifying the hashmap while enumerating its keys/values leads to corrupted data. */
hmError hmHashMapEnumerate(hmHashMap* hash_map, hmHashMapEnumerateFunc enumerate_func, void* user_data);
/* Gets the number of items actually contained in the hashmap. Implemented as a macro so must be OK to use as
   part of a condition in a loop. */
#define hmHashMapGetCount(hash_map) ((hash_map)->count)

#endif /* HM_HASHMAP_H */
