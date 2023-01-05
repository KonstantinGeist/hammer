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

#ifndef HM_TEST_H
#define HM_TEST_H

/* This file contains common macros to simplify testing and reduce boilerplate.
   See arrays.c for the example how to use OOM simulation. */

#include <core/common.h>

#include <assert.h> /* for HM_TEST_ macros */
#include <stdio.h>  /* for printf(..) */

typedef struct {
    const char* test_suite_name;
} hmTestSelector;

#define HM_PRE_STRINGIFY(s) #s
#define HM_STRINGIFY(s) HM_PRE_STRINGIFY(s)

#define HM_TEST_LOG(msg) printf("%s\n", msg)

#define HM_TEST_GLOBALS \
    static hm_nint hm_test_total_alloc_count __attribute__((unused)) = 0; \
    static hm_bool hm_test_is_oom_mode __attribute__((unused)) = HM_FALSE; \
    static hm_nint hm_test_oom_iteration __attribute__((unused)) = 0; \
    static hmAllocator hm_test_base_allocator __attribute__((unused)); \
    static hmAllocator* hm_test_oom_allocator __attribute__((unused)) = HM_NULL; \
    static hm_bool hm_test_is_oom __attribute__((unused)) = HM_FALSE; \
    static hm_nint hm_assert_count __attribute__((unused)) = 0;

#define HM_TEST_ON_FINALIZE hm_test_on_finalize:

#define HM_TEST_ASSERT(expr) assert(expr); hm_assert_count++
#define HM_TEST_ASSERT_OK(err) assert((err) == HM_OK); hm_assert_count++
#define HM_TEST_ASSERT_OK_OR_OOM(err) \
    if (!hm_test_is_oom_mode) { \
        assert((err) == HM_OK); \
        hm_assert_count++; \
    } else { \
        if (hmOOMAllocatorIsOutOfMEmory(hm_test_oom_allocator) && err == HM_ERROR_OUT_OF_MEMORY) { \
            hm_test_is_oom = HM_TRUE; \
            goto hm_test_on_finalize; \
        } \
        assert((err) == HM_OK); \
        hm_assert_count++; \
    }

#define HM_TEST_IS_OOM() (hm_test_is_oom_mode && hm_test_is_oom)

#define HM_TEST_INIT_ALLOC(allocator) \
    hm_test_is_oom = HM_FALSE; \
    if (!hm_test_is_oom_mode) { \
        hmError err = hmCreateSystemAllocator(&hm_test_base_allocator); \
        assert(err == HM_OK); \
        err = hmCreateStatsAllocator(&hm_test_base_allocator, allocator); \
        assert(err == HM_OK); \
    } else { \
        hmError err = hmCreateSystemAllocator(&hm_test_base_allocator); \
        assert(err == HM_OK); \
        err = hmCreateOOMAllocator(&hm_test_base_allocator, hm_test_oom_iteration, allocator); \
        assert(err == HM_OK); \
        hm_test_oom_allocator = allocator; \
    }

#define HM_TEST_DEINIT_ALLOC(allocator) \
    if (!hm_test_is_oom_mode) { \
        hm_test_total_alloc_count = hmStatsAllocatorGetTotalCount(allocator); \
        hmError err = hmAllocatorDispose(allocator); \
        assert(err == HM_OK); \
    } else { \
        hmError err = hmAllocatorDispose(allocator); \
        assert(err == HM_OK); \
    }

#define HM_TEST_TRACK_OOM(allocator, value) \
    if (!hm_test_is_oom_mode) { \
        hmStatsAllocatorTrackAllocCount(allocator, value); \
    } else { \
        hmOOMAllocatorTrackAllocCount(allocator, value); \
    }

/* Runs a test by keeping track of allocation count and then reruns it with the OOM allocator enabled. */
#define HM_TEST_RUN(name) \
    printf("    " HM_STRINGIFY(name) "\n"); \
    hm_test_is_oom_mode = HM_FALSE; \
    hm_assert_count = 0; \
    name(); \
    if (!hm_assert_count) { \
        printf("        SUSPICIOUS (no asserts)\n"); \
    } \
    hm_test_is_oom_mode = HM_TRUE; \
    if (hm_test_total_alloc_count > 0) { \
        printf("    " HM_STRINGIFY(name) "_OOM_%d_allocs\n", (int)hm_test_total_alloc_count); \
        for (hm_test_oom_iteration = 0; hm_test_oom_iteration < hm_test_total_alloc_count; hm_test_oom_iteration++) { \
            name(); \
        } \
    }

#define HM_TEST_RUN_WITHOUT_OOM(name) \
    hm_test_is_oom_mode = HM_FALSE; \
    printf("%s\n", "    " HM_STRINGIFY(name)); \
    hm_assert_count = 0; \
    name(); \
    if (!hm_assert_count) { \
        printf("        SUSPICIOUS (no asserts)\n"); \
    }

#define HM_TEST_DECLARE_SUITE(name) void test_ ## name();

#define HM_TEST_SUITE_BEGIN(name) void test_ ## name(hmTestSelector* test_selector) { \
    printf("%s\n", HM_STRINGIFY(name));

#define HM_TEST_SUITE_END() }
#define HM_TEST_RUN_SUITE(name) \
    if (!test_selector->test_suite_name || strcmp(test_selector->test_suite_name, HM_STRINGIFY(name)) == 0) \
        test_ ## name(test_selector);

#include <core/allocator.h>
HM_TEST_GLOBALS

#endif /* HM_TEST_H */
