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

#define HM_EMPTY_STRING_LENGTH_IN_BYTES HM_NINT_MAX

hmError hmCreateStringFromCString(hmAllocator* allocator, const char* content, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hm_nint length_in_bytes = strlen(content);
    hm_nint length_in_bytes_with_null = 0;
    HM_TRY(hmAddNint(length_in_bytes, 1, &length_in_bytes_with_null));
    hm_char* content_copy = (hm_char*)hmAlloc(allocator, length_in_bytes_with_null);
    if (!content_copy) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmCopyMemory(content_copy, content, length_in_bytes_with_null);
    in_string->content = content_copy;
    in_string->allocator_opt = allocator;
    in_string->length_in_bytes = length_in_bytes;
    return HM_OK;
}

hmError hmCreateStringFromCStringWithLengthInBytes(hmAllocator* allocator, const char* content, hm_nint length_in_bytes, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    if (!length_in_bytes) { /* special case as an optimization */
        return hmCreateEmptyStringView(in_string);
    }
    hm_nint length_in_bytes_with_null = 0;
    HM_TRY(hmAddNint(length_in_bytes, 1, &length_in_bytes_with_null));
    hm_char* content_copy = (hm_char*)hmAlloc(allocator, length_in_bytes_with_null);
    if (!content_copy) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmCopyMemory(content_copy, content, length_in_bytes);
    content_copy[length_in_bytes] = '\0'; /* null terminator */
    in_string->content = content_copy;
    in_string->allocator_opt = allocator;
    in_string->length_in_bytes = length_in_bytes;
    return HM_OK;
}

hmError hmCreateStringViewFromCString(const char* content, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    in_string->content = (hm_char*)content;
    in_string->allocator_opt = HM_NULL;
    in_string->length_in_bytes = HM_EMPTY_STRING_LENGTH_IN_BYTES; /* it will be computed lazily in hmStringGetLengthInBytes(..) */
    return HM_OK;
}

hmError hmCreateEmptyStringView(hmString* in_string)
{
    in_string->content = (hm_char*)"";
    in_string->allocator_opt = HM_NULL;
    in_string->length_in_bytes = 0;
    return HM_OK;
}

hmError hmStringDispose(hmString* string)
{
    if (string->allocator_opt) {
        hmFree(string->allocator_opt, string->content);
    }
    return HM_OK;
}

hm_bool hmStringEqualsToCString(hmString* string, const char* content)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    return strcmp((const char*)string->content, content) == 0;
}

hmError hmStringDuplicate(hmAllocator* allocator, hmString* string, hmString* in_duplicate)
{
    return hmCreateStringFromCString(allocator, (const char*)string->content, in_duplicate);
}

hm_bool hmStringEquals(hmString* string1, hmString* string2)
{
    return strcmp((const char*)string1->content, (const char*)string2->content) == 0;
}

hm_uint32 hmStringHash(hmString* string, hm_uint32 salt)
{
    return hmHash(string->content, hmStringGetLengthInBytes(string), salt);
}

hm_nint hmStringGetLengthInBytes(hmString* string)
{
    if (string->length_in_bytes != HM_EMPTY_STRING_LENGTH_IN_BYTES) {
        return string->length_in_bytes;
    }
    /* As HM_EMPTY_STRING_LENGTH_IN_BYTES is set to HM_NINT_MAX, which is a valid string length, there's a chance that
       it's a false positive and we'll end up recalculating the length over and over again; however, strings of size
       HM_NINT_MAX are extremely unlikely to exist in the wild. */
    string->length_in_bytes = strlen((const char*)string->content);
    return string->length_in_bytes;
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

hmComparisonResult hmStringCompareFunc(void* value1, void* value2, void* user_data)
{
    hmString* string1 = (hmString*)value1;
    hmString* string2 = (hmString*)value2;
    return hmStringCompare(string1, string2);
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
