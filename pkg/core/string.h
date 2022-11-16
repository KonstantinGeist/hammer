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

#ifndef HM_STRING_H
#define HM_STRING_H

#include <core/common.h>

struct _hmAllocator;

typedef struct {
    char*                content;
    struct _hmAllocator* allocator;
    hm_nint              length;    /* String's length is remembered to avoid O(n) lookups every time we need a string's length. */
    hm_uint32            hash;      /* Cached hash to speed up rehashing. */
} hmString;

/* Creates a Hammer string from a C string. Duplicates the given string and owns it: deallocates the internal buffer when the
   object is disposed of. See also hmCreateStringViewFromCString. */
hmError hmCreateStringFromCString(struct _hmAllocator* allocator, const char* content, hmString* in_string);
/* Creates a Hammer string from a C string. Unlike hmCreateStringFromCString (see), does not duplicate the string
   and does not own the internal buffer. The string view will be invalidated after the referenced string
   is deleted; it's undefined behavior to try to use such a string afterwards. Mostly useful for creating short-lived
   views for reading, for example, as a key to a container (if the container promises to never retain the value).
   String views should not be disposed, but it should be safe to try to dispose them. */
hmError hmCreateStringViewFromCString(const char* content, hmString* in_string);
hmError hmStringDuplicate(struct _hmAllocator* allocator, hmString* string, hmString* in_duplicate);
hmError hmStringDispose(hmString* string);
hm_bool hmStringEqualsToCString(hmString* string, const char* content);
hm_bool hmStringEquals(hmString* string1, hmString* string2);
hm_uint32 hmStringHash(hmString* string);
#define hmStringLength(string) (string)->length
#define hmStringContent(string) (string)->content

hm_uint32 hmStringHashFunc(void* key);
hm_bool hmStringEqualsFunc(void* value1, void* value2);
hmError hmStringDisposeFunc(void* obj);

/* Sometimes containers may need references to strings someone else owns. These hash/equals functions
   allow to operate on string references instead of strings themselves. */
hm_uint32 hmStringRefHashFunc(void* key);
hm_bool hmStringRefEqualsFunc(void* value1, void* value2);

#endif /* HM_STRING_H */