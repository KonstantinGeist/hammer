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

/* UTF8-related math expects chars to be unsigned, while our string's content is just `char*` for C interoperability,
   which is not guaranteed to be unsigned. So we define an explicitly signed UTF8 char type. */
typedef unsigned char hm_utf8char;

/* Allows to iterate over runes in a UTF8 string (UTF8 is a variable-sized encoding so you can't just increment the index
   when iterating):
   `out_rune` is the next decoded rune.
   `out_offset` is the size of the decoded rune which tells the offset for the next rune.
   If the returned offset is equal to 0, iteration is over.

    The function is to be called in a loop:

        while ((err = hmNextRune(content, length, &rune, &offset)) == HM_OK && offset > 0) {
            // Use the rune here.
            content += offset;
            length -= offset;
        }
*/
static hmError hmNextRune(const hm_utf8char* content, hm_nint length, hm_rune* out_rune, hm_nint* out_offset);
#define hmStringGetUTF8Chars(string) ((const hm_utf8char*)(string)->content)
#define hmIsContinuationUTF8Char(rune) (((rune) & 0xC0) == 0x80) /* to be used in hmNextRune(..) */

hmError hmCreateStringFromCString(hmAllocator* allocator, const char* content, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
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
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
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

hmError hmCreateStringViewFromCString(const char* content, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
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

hmError hmStringIndexRune(hmString* string, hm_rune rune_to_index, hm_nint* out_index)
{
    const hm_utf8char* content = hmStringGetUTF8Chars(string);
    hm_nint length = hmStringGetLengthInBytes(string);
    hmError err = HM_OK;
    hm_rune rune = 0;
    hm_nint offset = 0, index = 0;
    while ((err = hmNextRune(content, length, &rune, &offset)) == HM_OK && offset > 0) {
        if (rune == rune_to_index) {
            *out_index = index;
            return HM_OK;
        }
        content += offset;
        index += offset;
        length -= offset;
    }
    return err != HM_OK ? err : HM_ERROR_NOT_FOUND;
}

static hmError hmNextRune(const hm_utf8char* content, hm_nint length, hm_rune* out_rune, hm_nint* out_offset) {
    const hm_utf8char* content_end = content + length;
    if (!length) {
        *out_rune = 0;
        *out_offset = 0;
        return HM_OK;
    }
    hm_rune utf8_char = *content++;
    /* 1-byte sequence */
    if (utf8_char < 0x80) {
        *out_rune = utf8_char;
        *out_offset = 1;
        return HM_OK;
    }
    /* Must be between 0xC2 and 0xF4 inclusive to be valid. */
    if ((hm_uint32)(utf8_char - 0xC2) > (0xF4 - 0xC2)) {
        return HM_ERROR_INVALID_DATA;
    }
    /* 2-byte sequence */
    if (utf8_char < 0xE0) {
        if (content >= content_end /* must have 1 valid continuation character */
        || !hmIsContinuationUTF8Char(*content)) {
            return HM_ERROR_INVALID_DATA;
        }
        *out_rune = ((utf8_char & 0x1F) << 6) | (*content & 0x3F);
        *out_offset = 2;
        return HM_OK;
    }
    /* 3-byte sequence */
    if (utf8_char < 0xF0) {
        if ((content + 1 >= content_end) /* must have 2 valid continuation characters */
        || !hmIsContinuationUTF8Char(*content)
        || !hmIsContinuationUTF8Char(content[1])) {
            return HM_ERROR_INVALID_DATA;
        }
        /* Checks for surrogate chars. */
        if (utf8_char == 0xED && *content > 0x9F) {
            return HM_ERROR_INVALID_DATA;
        }
        utf8_char = ((utf8_char & 0xF) << 12) | ((*content & 0x3F) << 6) | (content[1] & 0x3F);
        if (utf8_char < 0x800) {
            return HM_ERROR_INVALID_DATA;
        }
        *out_rune = utf8_char;
        *out_offset = 3;
        return HM_OK;
    }
    /* 4-byte sequence */
    if ((content + 2 >= content_end) /* must have 3 valid continuation characters */
    || !hmIsContinuationUTF8Char(*content)
    || !hmIsContinuationUTF8Char(content[1])
    || !hmIsContinuationUTF8Char(content[2])) {
        return HM_ERROR_INVALID_DATA;
    }
    /* Is it in the correct range (0x10000 - 0x10FFFF)? */
    if (utf8_char == 0xF0) {
        if (*content < 0x90) {
            return HM_ERROR_INVALID_DATA;
        }
    } else if (utf8_char == 0xF4) {
        if (*content > 0x8F) {
            return HM_ERROR_INVALID_DATA;
        }
    }
    *out_rune = ((utf8_char & 7) << 18)
             | ((content[0] & 0x3F) << 12)
             | ((content[1] & 0x3F) << 6)
             |  (content[2] & 0x3F);
    *out_offset = 4;
    return HM_OK;
}
