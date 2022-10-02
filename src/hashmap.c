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

typedef struct _hmHashMapEntry {
    struct _hmHashMapEntry* next;
    char                    payload[1]; // the size afterwards depends on key_size+value_size
} hmHashMapEntry;

static hmError hmHashMapEntryDisposeFunc(void* object);

hmError hmCreateHashMap(
    struct _hmAllocator* allocator,
    hmHashMapHashFunc hash_func,
    hmHashMapEqualsFunc equals_func,
    hm_nint key_size,
    hm_nint value_size,
    hm_nint initial_capacity,
    hm_float threshold,
    hmHashMap* in_hashmap
)
{
    if (!hash_func || !equals_func || !initial_capacity || !threshold || threshold > 1.0) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmError err = hmCreateArray(
        allocator,
        sizeof(hmHashMapEntry*),
        initial_capacity,
        &hmHashMapEntryDisposeFunc,
        &in_hashmap->buckets
    );
    if (err != HM_OK) {
        return err;
    }
    err = hmArrayExpand(&in_hashmap->buckets, initial_capacity, HM_NULL, HM_NULL);
    if (err != HM_OK) {
        hmError err2 = hmArrayDispose(&in_hashmap->buckets);
        if (err2 != HM_OK) {
            err = err2;
        }
        return err;
    }
    in_hashmap->allocator = allocator;
    in_hashmap->hash_func = hash_func;
    in_hashmap->equals_func = equals_func;
    in_hashmap->key_size = key_size;
    in_hashmap->value_size = value_size;
    in_hashmap->threshold = threshold;
    return HM_OK;
}

hmError hmHashMapAdd(hmHashMap* hashMap, void* key, void* value)
{
    return HM_OK;
}

hmError hmHashMapGet(hmHashMap* hashMap, void* key, void** out_value)
{
    return HM_OK;
}

hmError hmHashMapDispose(hmHashMap* hashMap)
{
    return HM_OK;
}

static hmError hmHashMapEntryDisposeFunc(void* object)
{
    hmArray* entries = (hmArray*)object;
    return hmArrayDispose(entries);
}
