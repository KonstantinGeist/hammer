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

#include "common.h"
#include <core/string.h>

#include <string.h> /* for strlen(..) and strcmp(..) */

#define STRING_CONTENT "Hello, World!"
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
    HM_TEST_ASSERT(hmStringGetLength(&string) == strlen(STRING_CONTENT));
    HM_TEST_ASSERT(strcmp(hmStringGetRaw(&string), STRING_CONTENT) == 0);
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
    hmError err = hmCreateStringFromCStringWithLength(&allocator, STRING_CONTENT, strlen(STRING_CONTENT_TRIMMED), &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    HM_TEST_ASSERT(hmStringGetLength(&string) == strlen(STRING_CONTENT_TRIMMED));
    HM_TEST_ASSERT(strcmp(hmStringGetRaw(&string), STRING_CONTENT_TRIMMED) == 0);
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
    HM_TEST_ASSERT(hmStringGetLength(&string) == strlen(STRING_CONTENT));
    HM_TEST_ASSERT(strcmp(hmStringGetRaw(&string), STRING_CONTENT) == 0);
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
    HM_TEST_ASSERT(hmStringGetLength(&string) == hmStringGetLength(&duplicate));
    HM_TEST_ASSERT(strcmp(hmStringGetRaw(&string), hmStringGetRaw(&duplicate)) == 0);
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
    hmError err = hmCreateStringFromCStringWithLength(&allocator, "Hello,, World!", 0, &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    HM_TEST_ASSERT(hmStringGetLength(&string) == 0);
    HM_TEST_ASSERT(strcmp(hmStringGetRaw(&string), "") == 0);
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
    HM_TEST_ASSERT(hmStringGetLength(&string) == 0);
    HM_TEST_ASSERT(strcmp(hmStringGetRaw(&string), "") == 0);
    err = hmStringDispose(&string);
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
HM_TEST_SUITE_END()
