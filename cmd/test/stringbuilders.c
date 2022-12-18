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
#include <core/stringbuilder.h>
#include <core/string.h>

#include <string.h> /* for strcmp(..) */

static void test_can_create_string_builder_append_and_convert_to_string()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmStringBuilder string_builder;
    hmError err = hmCreateStringBuilder(&allocator, &string_builder);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    err = hmStringBuilderAppendCString(&string_builder, "Hello, ");
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmStringBuilderAppendCString(&string_builder, "World!");
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmString string;
    err = hmStringBuilderToString(&string_builder, HM_NULL, &string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(strcmp(hmStringGetRaw(&string), "Hello, World!") == 0);
    err = hmStringDispose(&string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
HM_TEST_ON_FINALIZE
    err = hmStringBuilderDispose(&string_builder);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_can_clear_string_builder()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmStringBuilder string_builder;
    hmError err = hmCreateStringBuilder(&allocator, &string_builder);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    err = hmStringBuilderAppendCString(&string_builder, "Hello, ");
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmStringBuilderAppendCString(&string_builder, "World!");
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmStringBuilderClear(&string_builder);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmStringBuilderAppendCString(&string_builder, "World!");
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmString string;
    err = hmStringBuilderToString(&string_builder, HM_NULL, &string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(strcmp(hmStringGetRaw(&string), "World!") == 0);
    err = hmStringDispose(&string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
HM_TEST_ON_FINALIZE
    err = hmStringBuilderDispose(&string_builder);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

void test_string_builders()
{
    HM_TEST_SUITE_BEGIN("StringBuilders");
        HM_TEST_RUN(test_can_create_string_builder_append_and_convert_to_string);
        HM_TEST_RUN(test_can_clear_string_builder);
    HM_TEST_SUITE_END();
}
