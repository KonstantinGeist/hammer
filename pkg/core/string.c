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
#include <core/utf8.h>
#include <core/utils.h>

#define HM_EMPTY_STRING_LENGTH_IN_BYTES HM_NINT_MAX

static hmError hmAddOffsetToUTF8Chars(const hm_utf8char* utf8_chars, hm_nint offset, const hm_utf8char** out_result);
#define hmStringGetUTF8Chars(string) ((const hm_utf8char*)(string)->content)

hmError hmCreateStringFromCString(hmAllocator* allocator, const char* content, hmString* in_string)
{
    hm_nint length_in_bytes = strlen(content);
    hm_nint length_in_bytes_with_null = 0;
    HM_TRY(hmAddNint(length_in_bytes, 1, &length_in_bytes_with_null));
    char* content_copy = (char*)hmAlloc(allocator, length_in_bytes_with_null);
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
    if (!length_in_bytes) { /* special case as an optimization */
        return hmCreateEmptyStringView(in_string);
    }
    hm_nint length_in_bytes_with_null = 0;
    HM_TRY(hmAddNint(length_in_bytes, 1, &length_in_bytes_with_null));
    char* content_copy = (char*)hmAlloc(allocator, length_in_bytes_with_null);
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

hmError hmCreateSubstring(hmAllocator* allocator, hmString* source, hm_nint start_index, hm_nint length_in_bytes, hmString* in_string)
{
    if (length_in_bytes == 0) {
        return hmCreateEmptyStringView(in_string);
    }
    hm_nint end_index = 0;
    HM_TRY(hmAddNint(start_index, length_in_bytes, &end_index));
    hm_nint source_length_in_bytes = hmStringGetLengthInBytes(source);
    if (start_index >= source_length_in_bytes || end_index > source_length_in_bytes) {
        return HM_ERROR_OUT_OF_RANGE;
    }
    hm_nint source_with_start_index = 0;
    HM_TRY(hmAddNint(hmCastPointerToNint(source->content), start_index, &source_with_start_index));
    return hmCreateStringFromCStringWithLengthInBytes(allocator, hmCastNintToPointer(source_with_start_index, const char*), length_in_bytes, in_string);
}

hmError hmCreateStringViewFromCString(const char* content, hmString* in_string)
{
    in_string->content = (char*)content;
    in_string->allocator_opt = HM_NULL;
    in_string->length_in_bytes = HM_EMPTY_STRING_LENGTH_IN_BYTES; /* it will be computed lazily in hmStringGetLengthInBytes(..) */
    return HM_OK;
}

hmError hmCreateEmptyStringView(hmString* in_string)
{
    in_string->content = (char*)"";
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
    return strcmp(string->content, content) == 0;
}

hm_bool hmStringStartsWithCStringAndLength(hmString* string, const char* prefix, hm_nint prefix_length)
{
    if (prefix_length > hmStringGetLengthInBytes(string)) {
        return HM_FALSE;
    }
    return strncmp(string->content, prefix, prefix_length) == 0;
}

hm_bool hmStringEndsWithCStringAndLength(hmString* string, const char* suffix, hm_nint suffix_length)
{
    hm_nint string_length_in_bytes = hmStringGetLengthInBytes(string);
    hm_nint offset = 0;
    /* If it underflows (`suffix_length > string_length_in_bytes`), returns HM_ERROR_UNDERFLOW.
       There's a check below for HM_OK which accounts for that. */
    hmError err = hmSubNint(string_length_in_bytes, suffix_length, &offset);
    if (err != HM_OK) {
        return HM_FALSE;
    }
    hm_nint c_string_offset = 0;
    err = hmAddNint(hmCastPointerToNint(string->content), offset, &c_string_offset);
    if (err != HM_OK) {
        return HM_FALSE;
    }
    return strncmp(hmCastNintToPointer(c_string_offset, const char*), suffix, suffix_length) == 0;
}

hm_bool hmStringStartsWithCString(hmString* string, const char* prefix)
{
    hm_nint prefix_length = strlen(prefix);
    return hmStringStartsWithCStringAndLength(string, prefix, prefix_length);
}

hm_bool hmStringEndsWithCString(hmString* string, const char* suffix)
{
    hm_nint suffix_length = strlen(suffix);
    return hmStringEndsWithCStringAndLength(string, suffix, suffix_length);
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

hmError hmStringIndexRune(hmString* string, hm_rune rune_to_index, hm_nint* out_index)
{
    const hm_utf8char* content = hmStringGetUTF8Chars(string);
    hm_nint length_in_bytes = hmStringGetLengthInBytes(string);
    hmError err = HM_OK;
    hm_rune rune = 0;
    hm_nint offset = 0, index = 0;
    while ((err = hmNextUTF8Rune(content, length_in_bytes, &rune, &offset)) == HM_OK && offset > 0) {
        if (rune == rune_to_index) {
            *out_index = index;
            return HM_OK;
        }
        HM_TRY(hmAddOffsetToUTF8Chars(content, offset, &content));;
        HM_TRY(hmAddNint(index, offset, &index));
        HM_TRY(hmSubNint(length_in_bytes, offset, &length_in_bytes));
    }
    return err != HM_OK ? err : HM_ERROR_NOT_FOUND;
}

static hmError hmAddOffsetToUTF8Chars(const hm_utf8char* utf8_chars, hm_nint offset, const hm_utf8char** out_result)
{
    hm_nint result;
    HM_TRY(hmAddNint(hmCastPointerToNint(utf8_chars), offset, &result));
    *out_result = hmCastNintToPointer(result, const hm_utf8char*);
    return HM_OK;
}
