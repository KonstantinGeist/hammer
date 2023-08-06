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

#include <core/stringpool.h>
#include <core/string.h>

hmError hmCreateStringPool(hmAllocator* allocator, hm_nint initial_capacity, hm_uint32 hash_salt, hmStringPool* in_pool)
{
    /* We use a bump pointer allocator for strings for two reasons:
        - we need to somehow store a string as both the key and the value and make sure both of them resolve
          to the same pointer, and we can't do it with the current hashmap design; so instead, we allocate strings
          externally (for the hashmap) in the string allocator;
        - the pool is not designed to shrink so far, so the most performant implementation in that case is a bump
          pointer allocator (cheap allocation, better memory locality).
        By setting the allocator's memory limit to HM_NINT_MAX we impose no limits but it still can be limited by
        the base allocator's own memory limits. */
    HM_TRY(hmCreateBumpPointerAllocator(allocator, HM_NINT_MAX, &in_pool->string_allocator));
    hmError err = hmCreateHashMap(
        allocator,
        &hmStringRefHashFunc,
        &hmStringRefEqualsFunc,
        HM_NULL,                 /* key_dispose_func -- no need to dispose because it's entirely owned by the string allocator which never shrinks */
        HM_NULL,                 /* value_dispose_func */
        sizeof(hmString*),
        sizeof(hmString*),
        initial_capacity,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        hash_salt,
        &in_pool->pool
    );
    if (err != HM_OK) {
        err = hmMergeErrors(err, hmAllocatorDispose(&in_pool->string_allocator));
    }
    return err;
}

hmError hmStringPoolDispose(hmStringPool* pool)
{
    hmError err = hmHashMapDispose(&pool->pool);
    return hmMergeErrors(err, hmAllocatorDispose(&pool->string_allocator));
}

/* NOTE: since the bump pointer allocator's memory space cannot be shrinked and hmFree(..) calls are no-ops (as per
   the documentation), we don't bother freeing strings on error here which simplifies the code. */
hmError hmStringPoolGet(hmStringPool* pool, hmString* in_string_view, hmString** out_string)
{
    hmError err = hmHashMapGet(&pool->pool, (void*)&in_string_view, &out_string);
    /* If the value is found -- just return it immediately. Also immediately returns if an unexpected error happened
       (HM_ERROR_NOT_FOUND is expected, on the other hand). */
    if (err == HM_OK || err != HM_ERROR_NOT_FOUND) {
        return err;
    }
    /* It's not a known string: see the comments in hmCreateStringPool(..) about how it works. */
    hmString* interned_string = (hmString*)hmAlloc(&pool->string_allocator, sizeof(hmString));
    if (!interned_string) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    HM_TRY(hmStringDuplicate(&pool->string_allocator, in_string_view, interned_string));
    HM_TRY(hmHashMapPut(&pool->pool, &interned_string, &interned_string));
    *out_string = interned_string;
    return HM_OK;
}

hm_nint hmStringPoolGetCount(hmStringPool* pool)
{
    return hmHashMapGetCount(&pool->pool);
}
