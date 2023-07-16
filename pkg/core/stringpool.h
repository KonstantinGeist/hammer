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

#ifndef HM_STRINGPOOL_H
#define HM_STRINGPOOL_H

#include <core/common.h>
#include <core/allocator.h>
#include <core/string.h>
#include <collections/hashmap.h>

#define HM_STRING_POOL_DEFAULT_CAPACITY 12000 /* an estimation by dividing HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE by sizeof(hmString) */

typedef struct {
    hmAllocator string_allocator; /* All strings are allocated from this pool. */
    hmHashMap   pool;             /* hmHashMap<hmString*, hmString*>, points to strings owned by string_allocator. */
} hmStringPool;

/* A string pool allows to save memory by reusing "interned" strings. For example, if something has N identical copies of
   a string, it's possible to share the same object N times instead of having N object copies.
   Useful, for example, for storing names of classes in TypeRef's.
   `initial_capacity` can be set to HM_STRING_POOL_DEFAULT_CAPACITY. Returns HM_ERROR_INVALID_ARGUMENT if it's zero.
   `hash_salt` is the hashing salt unique for the current runtime instance.
    Note that the minimum memory usage of the pool is similar to that of hmCreateBumpPointerAllocator(..) (see)
    which can measure in hundreds of kilobytes. */
hmError hmCreateStringPool(hmAllocator* allocator, hm_nint initial_capacity, hm_uint32 hash_salt, hmStringPool* in_pool);
hmError hmStringPoolDispose(hmStringPool* pool);
/* Receives a string view and searches the string in the pool. If it is already present, returns a pointer to the object
   stored in the pool (never try to dispose it manually because it's owned by the pool!) Otherwise, the input string is
   duplicated, saved inside the pool, and returned.
   If the pool is destroyed, all its strings are invalidated and cannot be used anymore. */
hmError hmStringPoolGet(hmStringPool* pool, hmString* in_string_view, hmString** out_string);
/* Returns the number of strings currently in the pool. Useful for debugging and in tests. */
hm_nint hmStringPoolGetCount(hmStringPool* pool);

#endif /* HM_STRINGPOOL_H */
