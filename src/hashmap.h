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

#define HM_DEFAULT_HASHMAP_CAPACITY 11
#define HM_DEFAULT_HASHMAP_THRESHOLD 0.75

struct _hmAllocator;

typedef int(*hmHashMapHashFunc)(void* key);
typedef hm_bool(*hmHashMapEqualsFunc)(void* value1, void* value2);

typedef struct {
    struct _hmAllocator* allocator;
    hmArray              buckets;  /* A list of buckets which contain entries: key_size+value_size (hmArray<hmArray<char*>>) */
    hmHashMapHashFunc    hash_func;
    hmHashMapEqualsFunc  equals_func;
    hm_nint              key_size;
    hm_nint              value_size;
    hm_nint              count;
    hm_float             threshold;
} hmHashMap;

hmError hmCreateHashMap(
    struct _hmAllocator* allocator,
    hmHashMapHashFunc hash_func,
    hmHashMapEqualsFunc equals_func,
    hm_nint key_size,
    hm_nint value_size,
    hm_nint initial_capacity,
    hm_float threshold,
    hmHashMap* in_hashmap
);
hmError hmHashMapAdd(hmHashMap* hashMap, void* key, void* value);
hmError hmHashMapGet(hmHashMap* hashMap, void* key, void** out_value);
hmError hmHashMapDispose(hmHashMap* hashMap);

#endif /* HM_HASHMAP_H */
