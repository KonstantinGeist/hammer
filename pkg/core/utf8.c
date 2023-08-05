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

#include <core/utf8.h>
#include <core/math.h>

static hmError hmAddOffsetToUTF8Chars(const hm_utf8char* utf8_chars, hm_nint offset, const hm_utf8char** out_result);
#define hmIsContinuationUTF8Char(rune) (((rune) & 0xC0) == 0x80) /* to be used in hmNextUTF8Rune(..) */
#define hmIsASCII(ch) ((ch) < 0x80)

hmError hmNextUTF8Rune(const hm_utf8char* content, hm_nint length_in_bytes, hm_rune* out_rune, hm_nint* out_offset)
{
    if (!length_in_bytes) {
        *out_rune = 0;
        *out_offset = 0;
        return HM_OK;
    }
    hm_int32 ch = *content;
    HM_TRY(hmAddOffsetToUTF8Chars(content, 1, &content));
    /* 1-byte sequence */
    if (hmIsASCII(ch)) {
        *out_rune = ch;
        *out_offset = 1;
        return HM_OK;
    }
    /* Must be between 0xC2 and 0xF4 inclusive to be valid. */
    if ((hm_uint32)(ch - 0xC2) > (0xF4 - 0xC2)) { /* no safe math for "- 0xC2" because `ch` is signed and it go below zero */
        return HM_ERROR_INVALID_DATA;
    }
    const hm_utf8char* content_end = HM_NULL;
    HM_TRY(hmAddOffsetToUTF8Chars(content, length_in_bytes, &content_end));
    /* 2-byte sequence */
    if (ch < 0xE0) {
        if (content >= content_end /* must have 1 valid continuation character */
        || !hmIsContinuationUTF8Char(*content)) {
            return HM_ERROR_INVALID_DATA;
        }
        *out_rune = ((ch & 0x1F) << 6) | (*content & 0x3F);
        *out_offset = 2;
        return HM_OK;
    }
    /* 3-byte sequence */
    if (ch < 0xF0) {
        const hm_utf8char* content_plus_one = HM_NULL;
        HM_TRY(hmAddOffsetToUTF8Chars(content, 1, &content_plus_one));
        if ((content_plus_one >= content_end) /* must have 2 valid continuation characters */
        || !hmIsContinuationUTF8Char(*content)
        || !hmIsContinuationUTF8Char(content[1])) {
            return HM_ERROR_INVALID_DATA;
        }
        /* Checks for surrogate chars. */
        if (ch == 0xED && *content > 0x9F) {
            return HM_ERROR_INVALID_DATA;
        }
        ch = ((ch & 0xF) << 12) | ((*content & 0x3F) << 6) | (content[1] & 0x3F);
        if (ch < 0x800) {
            return HM_ERROR_INVALID_DATA;
        }
        *out_rune = ch;
        *out_offset = 3;
        return HM_OK;
    }
    /* 4-byte sequence */
    const hm_utf8char* content_plus_two = HM_NULL;
    HM_TRY(hmAddOffsetToUTF8Chars(content, 2, &content_plus_two));
    if ((content_plus_two >= content_end) /* must have 3 valid continuation characters */
    || !hmIsContinuationUTF8Char(*content)
    || !hmIsContinuationUTF8Char(content[1])
    || !hmIsContinuationUTF8Char(content[2])) {
        return HM_ERROR_INVALID_DATA;
    }
    /* Is it in the correct range (0x10000 - 0x10FFFF)? */
    if (ch == 0xF0) {
        if (*content < 0x90) {
            return HM_ERROR_INVALID_DATA;
        }
    } else if (ch == 0xF4) {
        if (*content > 0x8F) {
            return HM_ERROR_INVALID_DATA;
        }
    }
    *out_rune = ((ch & 7) << 18)
             | ((content[0] & 0x3F) << 12)
             | ((content[1] & 0x3F) << 6)
             |  (content[2] & 0x3F);
    *out_offset = 4;
    return HM_OK;
}

static hmError hmAddOffsetToUTF8Chars(const hm_utf8char* utf8_chars, hm_nint offset, const hm_utf8char** out_result)
{
    hm_nint result = 0;
    HM_TRY(hmAddNint(hmCastPointerToNint(utf8_chars), offset, &result));
    *out_result = hmCastNintToPointer(result, const hm_utf8char*);
    return HM_OK;
}
