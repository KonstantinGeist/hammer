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

#include "common.h"

struct _hmAllocator;

typedef struct {
    char*                content;
    struct _hmAllocator* allocator;
    hm_nint              length;     /* String's length is remembered to avoid O(n) lookups every time we need a string's length. */
} hmString;

hmError hmCreateStringFromCString(struct _hmAllocator* allocator, const char* content, hmString* in_string);
hmError hmStringDuplicate(hmString* string, hmString* in_duplicate);
hmError hmStringDispose(hmString* string);
hm_bool hmStringEqualsToCString(hmString* string, const char* content);
hm_bool hmStringEquals(hmString* string1, hmString* string2);
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
