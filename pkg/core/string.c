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

#include <core/string.h>
#include <core/allocator.h>
#include <core/hash.h>

#define HM_EMPTY_STRING_HASH HM_UINT32_MAX

hmError hmCreateStringFromCString(hmAllocator* allocator, const char* content, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hm_nint length = strlen(content);
    char* content_copy = hmAlloc(allocator, length+1); // including null terminator
    if (!content_copy) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    memcpy(content_copy, content, length+1); // including null terminator
    in_string->content = content_copy;
    in_string->allocator = allocator;
    in_string->length = length;
    in_string->hash = HM_EMPTY_STRING_HASH;
    return HM_OK;
}

hmError hmCreateStringViewFromCString(const char* content, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    in_string->content = (char*)content;
    in_string->allocator = HM_NULL;
    in_string->length = strlen(content);
    in_string->hash = HM_EMPTY_STRING_HASH;
    return HM_OK;
}

hmError hmStringDispose(hmString* string)
{
    if (string->allocator) {
        hmFree(string->allocator, string->content);
    }
    return HM_OK;
}

hm_bool hmStringEqualsToCString(hmString* string, const char* content)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    return strcmp(string->content, content) == 0;
}

hmError hmStringDuplicate(hmAllocator* allocator, hmString* string, hmString* in_duplicate)
{
    return hmCreateStringFromCString(allocator, string->content, in_duplicate);
}

hm_bool hmStringEquals(hmString* string1, hmString* string2)
{
    if (string1->length != string2->length) {
        return HM_FALSE;
    }
    return strcmp(string1->content, string2->content) == 0;
}

hm_uint32 hmStringHash(hmString* string)
{
    if (string->hash != HM_EMPTY_STRING_HASH) {
        return string->hash;
    }
    hm_uint32 hash = hmHash(string->content, string->length);
    // hash should never be HM_EMPTY_STRING_HASH because it signifies "no hash computed"
    if (hash == HM_EMPTY_STRING_HASH) {
        hash++;
    }
    string->hash = hash;
    return string->hash;
}

hm_uint32 hmStringHashFunc(void* key)
{
    hmString* string = (hmString*)key;
    return hmStringHash(string);
}

hm_bool hmStringEqualsFunc(void* value1, void* value2)
{
    hmString* string1 = (hmString*)value1;
    hmString* string2 = (hmString*)value2;
    return hmStringEquals(string1, string2);
}

hmError hmStringDisposeFunc(void* obj)
{
    hmString* string = (hmString*)obj;
    return hmStringDispose(string);
}

hm_uint32 hmStringRefHashFunc(void* key)
{
    hmString* string = *((hmString**)key);
    return hmStringHash(string);
}

hm_bool hmStringRefEqualsFunc(void* value1, void* value2)
{
    hmString* string1 = *((hmString**)value1);
    hmString* string2 = *((hmString**)value2);
    return hmStringEquals(string1, string2);
}
