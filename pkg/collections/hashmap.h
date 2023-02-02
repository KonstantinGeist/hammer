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
    struct hmHashMapEntry_** buckets;  /* A list of buckets which contain linked lists of entries of size key_size + value_size. */
    hmHashMapHashFunc        hash_func_opt;
    hmHashMapEqualsFunc      equals_func_opt;
    hmDisposeFunc            key_dispose_func_opt;   /* can be HM_NULL */
    hmDisposeFunc            value_dispose_func_opt; /* can be HM_NULL */
    hm_nint                  key_size;
    hm_nint                  value_size;
    hm_nint                  count;
    hm_nint                  bucket_count;
    hm_float64               threshold;
    hm_float64               load_factor;
    hm_uint32                hash_salt;
} hmHashMap;

/* Creates a hash map, with provided hash_func, equals_func, key/value sizes.
   Load factor should be in the range [0.5, 1.0] (preferred value is HM_HASHMAP_DEFAULT_LOAD_FACTOR).
   Initial capacity can be set to HM_HASHMAP_DEFAULT_CAPACITY. Returns HM_ERROR_INVALID_ARGUMENT if it's zero.
   key_dispose_func and value_dispose_func can be HM_NULL (nothing will be disposed in that case).
   hash_func and equals_func can be HM_NULL (in that case, bitwise comparisons are made).
   hash_salt is used to salt hashes to prevent against hash DoS attacks, see hmHash(..) for more details.
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
/* Puts a value in the map by the given key. Note that if dispose_func is provided, it's always called on the old value,
   even if it's the same value -- which can lead to use-after-free if used without care.
   `key` and `value` are pointers to values, not the values themselves. */
hmError hmHashMapPut(hmHashMap* hash_map, void* key, void* value);
/* Tries to retrieve an element from the map. Returns HM_ERROR_NOT_FOUND if no element by the given key is found.
   Never retains the key value (safe to use with views). The returned value is copied by value; that is, it's not safe
   to delete such an object if its ownership belongs to the hashmap. Use hmHashMapGetRef if you need a reference. */
hmError hmHashMapGet(hmHashMap* hash_map, void* key, void* in_value);
/* Same as hmHashMapGet, except returns a pointer to the value directly as stored in the hashmap, instead of copying it by value.
   The value is stable after rehashing. However, the reference will point to a different object if a different value is
   put in the map with the same key. The reference is invalidated when it's removed from the map. */
hmError hmHashMapGetRef(hmHashMap* hash_map, void* key, void** in_value);
hm_bool hmHashMapContains(hmHashMap* hash_map, void* key);
/* Removes an item from the map, by the given key. Returns out_removed, if the element was actually removed.
   out_removed can be HM_NULL. */
hmError hmHashMapRemove(hmHashMap* hash_map, void* key, hm_bool* out_removed_opt);
/* Enumerates all the keys and values in the map by calling function `enumerate_func`. The error returned from `enumerate_func`
   is returned from hmHashMapEnumerate(..) as is. On any error other than HM_OK, enumeration is immediately terminated.
   Use `user_data` to pass additional context to the callback.
   Iteration order is not guaranteed.
   WARNING Modifying the hashmap while enumerating its keys/values leads to corrupted data. */
hmError hmHashMapEnumerate(hmHashMap* hash_map, hmHashMapEnumerateFunc enumerate_func, void* user_data);
/* Moves all key/value pairs of the hashmap to `in_dest_hash_map`. The original hashmap becomes empty. No user-defined
   dispose functions are called because the values are still alive (just ownership changed).
   Useful for implementing atomic operations "everything or nothing", i.e. keys/values are stored in a temporary map
   and moved to the final map only if the whole operation (which may include different data structures) is successful.
   The operation is guaranteed to fully complete (it's rolled back on an error).
   Both `hash_map` and `in_dest_hash_map` must store the same key/value types and same dispose functions, or crashes/memory leaks
   may occur. If some of the keys in `in_dest_hash_map` also exist in `hash_map` before the operation, HM_ERROR_INVALID_ARGUMENT
   is returned. */
hmError hmHashMapMoveTo(hmHashMap* hash_map, hmHashMap* in_dest_hash_map);
#define hmHashMapGetCount(hash_map) ((hash_map)->count)

#endif /* HM_HASHMAP_H */
