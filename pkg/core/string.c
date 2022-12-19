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

#include <core/string.h>
#include <core/allocator.h>
#include <core/hash.h>
#include <core/math.h>
#include <core/utils.h>

#define HM_EMPTY_STRING_HASH HM_UINT32_MAX
#define HM_EMPTY_STRING_LENGTH HM_NINT_MAX

hmError hmCreateStringFromCString(hmAllocator* allocator, const char* content, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hm_nint length = strlen(content);
    hm_nint length_with_null = 0;
    HM_TRY(hmAddNint(length, 1, &length_with_null));
    char* content_copy = (char*)hmAlloc(allocator, length_with_null);
    if (!content_copy) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmCopyMemory(content_copy, content, length_with_null);
    in_string->content = content_copy;
    in_string->allocator = allocator;
    in_string->length = length;
    in_string->hash = HM_EMPTY_STRING_HASH;
    return HM_OK;
}

hmError hmCreateStringFromCStringAndLength(struct _hmAllocator* allocator, const char* content, hm_nint length, hmString* in_string)
{
    if (!content || !length) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hm_nint length_with_null = 0;
    HM_TRY(hmAddNint(length, 1, &length_with_null));
    char* content_copy = (char*)hmAlloc(allocator, length_with_null);
    if (!content_copy) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmCopyMemory(content_copy, content, length);
    content_copy[length] = '\0'; /* null terminator */
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
    in_string->length = HM_EMPTY_STRING_LENGTH; /* it will be computed lazily in hmStringGetLength(..) */
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
    return strcmp(string1->content, string2->content) == 0;
}

hm_uint32 hmStringHash(hmString* string, hm_uint32 salt)
{
    if (string->hash != HM_EMPTY_STRING_HASH) {
        return string->hash;
    }
    hm_uint32 hash = hmHash(string->content, hmStringGetLength(string), salt);
    /* Hash should never be HM_EMPTY_STRING_HASH because it signifies "no hash computed". */
    if (hash == HM_EMPTY_STRING_HASH) {
        hash++; /* OK to wrap around, because it's an unsigned value */
    }
    string->hash = hash;
    return string->hash;
}

hm_nint hmStringGetLength(hmString* string)
{
    if (string->length != HM_EMPTY_STRING_LENGTH) {
        return string->length;
    }
    /* As HM_EMPTY_STRING_LENGTH is set to HM_NINT_MAX, which is a valid string length, there's a chance that
       it's a false positive and we'll end up recalculating the length over and over again; however, strings of size
       HM_NINT_MAX are extremely unlikely to exist in the wild. */
    string->length = strlen(string->content);
    return string->length;
}

hm_uint32 hmStringHashFunc(void* key, hm_uint32 salt)
{
    hmString* string = (hmString*)key;
    return hmStringHash(string, salt);
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

hm_uint32 hmStringRefHashFunc(void* key, hm_uint32 salt)
{
    hmString* string = *((hmString**)key);
    return hmStringHash(string, salt);
}

hm_bool hmStringRefEqualsFunc(void* value1, void* value2)
{
    hmString* string1 = *((hmString**)value1);
    hmString* string2 = *((hmString**)value2);
    return hmStringEquals(string1, string2);
}
