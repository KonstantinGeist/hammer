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

#include "../common.h"
#include <core/string.h>

#include <string.h> /* for strlen(..) */

#define STRING_CONTENT "Hello, World!"
#define STRING_CONTENT_IN_CYRILLIC "Привет, мир!"
#define STRING_CONTENT_TRIMMED "Hello"
#define DIFFERENT_STRING_CONTENT "different string content"
#define HASH_SALT 34545

static void test_can_create_string_from_c_string()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmString string;
    hmError err = hmCreateStringFromCString(&allocator, STRING_CONTENT, &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    HM_TEST_ASSERT(hmStringGetLengthInBytes(&string) == strlen(STRING_CONTENT));
    HM_TEST_ASSERT(hmStringEqualsToCString(&string, STRING_CONTENT));
    err = hmStringDispose(&string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
HM_TEST_ON_FINALIZE
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_can_create_string_from_c_string_and_length()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmString string;
    hmError err = hmCreateStringFromCStringWithLengthInBytes(&allocator, STRING_CONTENT, strlen(STRING_CONTENT_TRIMMED), &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    HM_TEST_ASSERT(hmStringGetLengthInBytes(&string) == strlen(STRING_CONTENT_TRIMMED));
    HM_TEST_ASSERT(hmStringEqualsToCString(&string, STRING_CONTENT_TRIMMED));
    err = hmStringDispose(&string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
HM_TEST_ON_FINALIZE
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_can_create_string_view()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT, &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringGetLengthInBytes(&string) == strlen(STRING_CONTENT));
    HM_TEST_ASSERT(hmStringEqualsToCString(&string, STRING_CONTENT));
    err = hmStringDispose(&string); /* not necessary for views; just checking it doesn't crash */
    HM_TEST_ASSERT_OK(err);
}

static void test_can_duplicate_string()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT, &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hmString duplicate;
    err = hmStringDuplicate(&allocator, &string, &duplicate);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(hmStringGetLengthInBytes(&string) == hmStringGetLengthInBytes(&duplicate));
    HM_TEST_ASSERT(hmStringEqualsToCString(&string, hmStringGetCString(&duplicate)));
    err = hmStringDispose(&duplicate);
    HM_TEST_ASSERT_OK_OR_OOM(err);
HM_TEST_ON_FINALIZE
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_can_compare_string_to_c_string()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT, &string);
    HM_TEST_ASSERT_OK(err);
    hm_bool b = hmStringEqualsToCString(&string, STRING_CONTENT);
    HM_TEST_ASSERT(b);
    b = hmStringEqualsToCString(&string, DIFFERENT_STRING_CONTENT);
    HM_TEST_ASSERT(!b);
}

static void test_can_compare_strings()
{
    hmString string1;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT, &string1);
    HM_TEST_ASSERT_OK(err);
    hmString string2;
    err = hmCreateStringViewFromCString(STRING_CONTENT, &string2);
    HM_TEST_ASSERT_OK(err);
    hmString string3;
    err = hmCreateStringViewFromCString(DIFFERENT_STRING_CONTENT, &string3);
    HM_TEST_ASSERT_OK(err);
    hm_bool b = hmStringEquals(&string1, &string1);
    HM_TEST_ASSERT(b);
    b = hmStringEquals(&string1, &string3);
    HM_TEST_ASSERT(!b);
}

static void test_can_hash_string()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT, &string);
    HM_TEST_ASSERT_OK(err);
    hm_uint32 hash = hmStringHash(&string, HASH_SALT);
    HM_TEST_ASSERT(hash == 1485836977); /* precomputed */
}

static void test_can_hash_empty_string()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString("", &string);
    HM_TEST_ASSERT_OK(err);
    hm_uint32 hash = hmStringHash(&string, HASH_SALT);
    HM_TEST_ASSERT(hash == 34545); /* precomputed */
}

static void test_can_create_string_with_zero_length()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmString string;
    hmError err = hmCreateStringFromCStringWithLengthInBytes(&allocator, "Hello,, World!", 0, &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    HM_TEST_ASSERT(hmStringGetLengthInBytes(&string) == 0);
    HM_TEST_ASSERT(hmStringEqualsToCString(&string, ""));
    err = hmStringDispose(&string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
HM_TEST_ON_FINALIZE
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_can_create_empty_string_view()
{
    hmString string;
    hmError err = hmCreateEmptyStringView(&string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringGetLengthInBytes(&string) == 0);
    HM_TEST_ASSERT(hmStringEqualsToCString(&string, ""));
    err = hmStringDispose(&string);
    HM_TEST_ASSERT_OK(err);
}

static void test_different_salt_returns_different_string_hashes()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT, &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringHash(&string, 0) != hmStringHash(&string, 1));
}

static void test_can_index_rune_in_string_in_latin()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT, &string);
    HM_TEST_ASSERT_OK(err);
    hm_nint index = 0;
    err = hmStringIndexRune(&string, (hm_rune)'W', &index);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(index == 7);
}

static void test_can_index_rune_in_string_in_cyrillic()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT_IN_CYRILLIC, &string);
    HM_TEST_ASSERT_OK(err);
    hm_nint index = 0;
    err = hmStringIndexRune(&string, (hm_rune)0x043C, &index); /* tries to find character "CYRILLIC SMALL LETTER EM" */
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(index == 14);
}

static void test_index_rune_returns_not_found_error()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT_IN_CYRILLIC, &string);
    HM_TEST_ASSERT_OK(err);
    hm_nint index = 0;
    err = hmStringIndexRune(&string, (hm_rune)'z', &index);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
}

static void test_index_rune_expects_empty_strings()
{
    hmString string;
    hmError err = hmCreateEmptyStringView(&string);
    HM_TEST_ASSERT_OK(err);
    hm_nint index = 0;
    err = hmStringIndexRune(&string, (hm_rune)'z', &index);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
}

static void test_can_index_last_rune()
{
    hmString string;
    hmError err = hmCreateStringViewFromCString(STRING_CONTENT_IN_CYRILLIC, &string);
    HM_TEST_ASSERT_OK(err);
    hm_nint index = 0;
    err = hmStringIndexRune(&string, (hm_rune)'!', &index);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(index == 20);
}

static void test_index_rune_returns_invalid_data_error()
{
    unsigned char chars[3] = { 0xC4, 0x0A, 0x0 };
    hmString string;
    hmError err = hmCreateStringViewFromCString((const char*)chars, &string);
    HM_TEST_ASSERT_OK(err);
    hm_nint index = 0;
    err = hmStringIndexRune(&string, (hm_rune)'!', &index);
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_DATA);
}

HM_TEST_SUITE_BEGIN(strings)
    HM_TEST_RUN(test_can_create_string_from_c_string)
    HM_TEST_RUN(test_can_create_string_from_c_string_and_length)
    HM_TEST_RUN(test_can_create_string_view)
    HM_TEST_RUN(test_can_duplicate_string)
    HM_TEST_RUN(test_can_compare_string_to_c_string)
    HM_TEST_RUN(test_can_compare_strings)
    HM_TEST_RUN(test_can_hash_string)
    HM_TEST_RUN(test_can_hash_empty_string)
    HM_TEST_RUN(test_can_create_string_with_zero_length)
    HM_TEST_RUN(test_can_create_empty_string_view);
    HM_TEST_RUN(test_different_salt_returns_different_string_hashes);
    HM_TEST_RUN(test_can_index_rune_in_string_in_latin)
    HM_TEST_RUN(test_can_index_rune_in_string_in_cyrillic)
    HM_TEST_RUN(test_index_rune_returns_not_found_error)
    HM_TEST_RUN(test_index_rune_expects_empty_strings)
    HM_TEST_RUN(test_can_index_last_rune)
    HM_TEST_RUN(test_index_rune_returns_invalid_data_error)
HM_TEST_SUITE_END()
