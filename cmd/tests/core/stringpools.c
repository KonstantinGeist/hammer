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
#include <core/stringpool.h>

#define HASHMAP_DEFAULT_CAPACITY 4 /* a small value to trigger rehashes more often */
#define HASH_SALT 666
#define ITERATION_COUNT 8

static const char* test_strings[ITERATION_COUNT] = {
    "Lorem ipsum", "dolor sit amet", "consectetur adipiscing elit", "sed do eiusmod tempor incididunt",
    "ut labore et dolore magna aliqua", "Ut enim ad minim veniam", "quis nostrud exercitation ullamco laboris",
    "nisi ut aliquip ex"
};

static void test_can_create_string_pool()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    hmStringPool pool;
    hm_bool is_pool_initialized = HM_FALSE;
    hmError err = hmCreateStringPool(&allocator, HASHMAP_DEFAULT_CAPACITY, HASH_SALT, &pool);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_pool_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringPoolGetCount(&pool) == 0);
HM_TEST_ON_FINALIZE
    if (is_pool_initialized) {
        err = hmStringPoolDispose(&pool);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_string_pool_can_be_filled_with_many_strings()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmStringPool pool;
    hmError err = hmCreateStringPool(&allocator, HASHMAP_DEFAULT_CAPACITY, HASH_SALT, &pool);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) {
        hmString string_view;
        err = hmCreateStringViewFromCString(test_strings[i], &string_view);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        hmString* interned_string_ref = HM_NULL;
        err = hmStringPoolGetRef(&pool, &string_view, &interned_string_ref);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(hmStringEquals(&string_view, interned_string_ref));
    }
    HM_TEST_ASSERT(hmStringPoolGetCount(&pool) == ITERATION_COUNT);
HM_TEST_ON_FINALIZE
    err = hmStringPoolDispose(&pool);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_string_pool_returns_same_string()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmStringPool pool;
    hmError err = hmCreateStringPool(&allocator, HASHMAP_DEFAULT_CAPACITY, HASH_SALT, &pool);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hmString string_view;
    err = hmCreateStringViewFromCString(test_strings[0], &string_view);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmString* interned_string_ref = HM_NULL;
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) {
        err = hmStringPoolGetRef(&pool, &string_view, &interned_string_ref);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(hmStringEquals(&string_view, interned_string_ref));
    }
    HM_TEST_ASSERT(hmStringPoolGetCount(&pool) == 1);
HM_TEST_ON_FINALIZE
    err = hmStringPoolDispose(&pool);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

HM_TEST_SUITE_BEGIN(string_pools)
    HM_TEST_RUN(test_can_create_string_pool)
    HM_TEST_RUN(test_string_pool_can_be_filled_with_many_strings)
    HM_TEST_RUN(test_string_pool_returns_same_string)
HM_TEST_SUITE_END()
