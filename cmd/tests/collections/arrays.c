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
#include <collections/array.h>

#include <string.h> /* for strlen(..) */

#define ARRAY_CAPACITY     4
#define ARRAY_EXPAND_COUNT 100 /* big enough to also test reallocation */

typedef struct {
    hm_nint x;
    hm_nint y;
} testItem;

static hm_nint item_dispose_sum = 0;

static void create_array_and_allocator(hmArray* array, hmAllocator* allocator, hmDisposeFunc item_dispose_func)
{
    HM_TEST_INIT_ALLOC(allocator);
    HM_TEST_TRACK_OOM(allocator, HM_FALSE);
    hmError err = hmCreateArray(allocator, sizeof(testItem), ARRAY_CAPACITY, item_dispose_func, array);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(allocator, HM_TRUE);
}

static void dispose_array_and_allocator(hmArray* array, hmAllocator* allocator)
{
    hmError err = hmArrayDispose(array);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(allocator);
}

static void test_array_can_create_add_get_dispose_without_item_dispose_func()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, HM_NULL);
    for (hm_nint i = 0; i < ARRAY_CAPACITY+5; i++) { /* note: also checks reallocations */
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        testItem retrieved_item;
        err = hmArrayGet(&array, i, &retrieved_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(test_item.x == retrieved_item.x);
        HM_TEST_ASSERT(test_item.y == retrieved_item.y);
    }
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
}

static hmError item_dispose_func(void* value)
{
    testItem* test_item = (testItem*)value;
    item_dispose_sum += test_item->x+test_item->y;
    return HM_OK;
}

static void test_array_can_create_add_get_dispose_with_item_dispose_func()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    hm_nint item_dispose_sum_control = 0;
    item_dispose_sum = 0;
    for (hm_nint i = 0; i < ARRAY_CAPACITY*2+1; i++) { /* note: also checks reallocations */
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        item_dispose_sum_control += test_item.x+test_item.y;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
    if (!HM_TEST_IS_OOM()) {
        HM_TEST_ASSERT(item_dispose_sum == item_dispose_sum_control);
    }
}

static void test_returns_error_if_get_out_of_range()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    testItem test_item;
    test_item.x = 10;
    test_item.y = 20;
    hmError err = hmArrayAdd(&array, &test_item);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmArrayGet(&array, 2, &test_item);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_RANGE);
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
}

static void test_returns_error_if_set_out_of_range()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    testItem test_item;
    test_item.x = 10;
    test_item.y = 20;
    hmError err = hmArraySet(&array, 17, &test_item);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_RANGE);
    dispose_array_and_allocator(&array, &allocator);
}

static void test_can_iterate_over_raw_array()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    for (hm_nint i = 0; i < ARRAY_CAPACITY; i++) {
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    testItem* raw_items = hmArrayGetRaw(&array, testItem);
    for (hm_nint i = 0; i < hmArrayGetCount(&array); i++) {
        testItem control_item;
        control_item.x = i*10;
        control_item.y = i*20;
        testItem retrieved_item = raw_items[i];
        HM_TEST_ASSERT(control_item.x == retrieved_item.x);
        HM_TEST_ASSERT(control_item.y == retrieved_item.y);
    }
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
}

static void test_can_expand_array_without_expand_func()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    for (hm_nint i = 0; i < ARRAY_CAPACITY; i++) {
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    hmError err = hmArrayExpand(&array, ARRAY_EXPAND_COUNT, HM_NULL, HM_NULL);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(hmArrayGetCount(&array) == ARRAY_CAPACITY+ARRAY_EXPAND_COUNT);
    testItem* test_item = hmArrayGetRaw(&array, testItem)+ARRAY_CAPACITY;
    for (hm_nint i = 0; i < ARRAY_EXPAND_COUNT; i++) {
        HM_TEST_ASSERT(test_item->x == 0);
        HM_TEST_ASSERT(test_item->y == 0);
        test_item++;
    }
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
}

static hmError array_expand_func(hmArray* array, hm_nint index, void* in_item, void* user_data)
{
    testItem* test_item = (testItem*)in_item;
    hm_nint base_int = *((hm_nint*)user_data);
    test_item->x = base_int+index*10;
    test_item->y = base_int+index*20;
    return HM_OK;
}

static void test_can_expand_array_with_expand_func()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    for (hm_nint i = 0; i < ARRAY_CAPACITY; i++) {
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    hm_nint base_int = 666;
    hmError err = hmArrayExpand(&array, ARRAY_EXPAND_COUNT, &array_expand_func, &base_int);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(hmArrayGetCount(&array) == ARRAY_CAPACITY+ARRAY_EXPAND_COUNT);
    testItem* test_item = hmArrayGetRaw(&array, testItem)+ARRAY_CAPACITY;
    for (hm_nint i = 0; i < ARRAY_EXPAND_COUNT; i++) {
        HM_TEST_ASSERT(test_item->x == base_int+(i+ARRAY_CAPACITY)*10);
        HM_TEST_ASSERT(test_item->y == base_int+(i+ARRAY_CAPACITY)*20);
        test_item++;
    }
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
}

static void test_can_set_array_item()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    hmError err = hmArrayExpand(&array, 4, HM_NULL, HM_NULL);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    testItem test_item;
    test_item.x = 13;
    test_item.y = 666;
    err = hmArraySet(&array, 2, &test_item);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    testItem retrieved_item;
    err = hmArrayGet(&array, 2, &retrieved_item);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(test_item.x == retrieved_item.x);
    HM_TEST_ASSERT(test_item.y == retrieved_item.y);
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
}

static void test_can_add_range_to_array()
{
    #define ADD_RANGE_COUNT (ARRAY_CAPACITY - 1) /* to make sure AddRange exceeds the capacity and we have a resizing */
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    for (hm_nint i = 0; i < ADD_RANGE_COUNT; i++) {
        testItem test_item;
        test_item.x = 10 * i;
        test_item.y = 20 * i;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    testItem test_items[ADD_RANGE_COUNT];
    for (hm_nint i = 0; i < ADD_RANGE_COUNT; i++) {
        testItem test_item;
        test_item.x = 10 * (ADD_RANGE_COUNT + i);
        test_item.y = 20 * (ADD_RANGE_COUNT + i);
        test_items[i] = test_item;
    }
    hmError err = hmArrayAddRange(&array, test_items, ADD_RANGE_COUNT);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    for (hm_nint i = 0; i < ADD_RANGE_COUNT * 2; i++) {
        testItem test_item;
        err = hmArrayGet(&array, i, &test_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(test_item.x == i * 10);
        HM_TEST_ASSERT(test_item.y == i * 20);
    }
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
}

static void test_can_add_range_to_array_with_new_count_exceeding_capacity_greater_than_growth_factor()
{
    #define ITEM_OS_NAME "Linux"
    #define ITEM_SPACE " "
    #define ITEM_KERNEL_VERSION "5.15.0-57-generic"
    #define ITEM_KERNEL_BUILD "#63~20.04.1-Ubuntu SMP Wed Nov 30 13:40:16 UTC 2022"
    #define ITEM_KERNEL_ARCH "x86_64"
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    hmError err = hmArrayAddRange(&array, ITEM_OS_NAME, strlen(ITEM_OS_NAME));
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmArrayAddRange(&array, ITEM_SPACE, strlen(ITEM_SPACE));
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmArrayAddRange(&array, ITEM_KERNEL_VERSION, strlen(ITEM_KERNEL_VERSION));
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmArrayAddRange(&array, ITEM_SPACE, strlen(ITEM_SPACE));
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmArrayAddRange(&array, ITEM_KERNEL_BUILD, strlen(ITEM_KERNEL_BUILD));
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmArrayAddRange(&array, ITEM_SPACE, strlen(ITEM_SPACE));
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmArrayAddRange(&array, ITEM_KERNEL_ARCH, strlen(ITEM_KERNEL_ARCH));
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hm_nint expected_length = strlen(ITEM_OS_NAME)
        + strlen(ITEM_KERNEL_VERSION)
        + strlen(ITEM_KERNEL_BUILD)
        + strlen(ITEM_KERNEL_ARCH)
        + strlen(ITEM_SPACE) * 3;
    HM_TEST_ASSERT(hmArrayGetCount(&array) == expected_length);
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
}

static void test_can_clear_array()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    hm_nint item_dispose_sum_control = 0;
    item_dispose_sum = 0;
    for (hm_nint i = 0; i < ARRAY_CAPACITY*2+1; i++) { /* note: the test also checks reallocations */
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        item_dispose_sum_control += test_item.x+test_item.y;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    hmError err = hmArrayClear(&array);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(item_dispose_sum == item_dispose_sum_control);
HM_TEST_ON_FINALIZE
    dispose_array_and_allocator(&array, &allocator);
    if (!HM_TEST_IS_OOM()) {
        HM_TEST_ASSERT(item_dispose_sum == item_dispose_sum_control);
    }
}

HM_TEST_SUITE_BEGIN(arrays)
    HM_TEST_RUN(test_array_can_create_add_get_dispose_without_item_dispose_func)
    HM_TEST_RUN(test_array_can_create_add_get_dispose_with_item_dispose_func)
    HM_TEST_RUN(test_returns_error_if_get_out_of_range)
    HM_TEST_RUN(test_returns_error_if_set_out_of_range)
    HM_TEST_RUN(test_can_iterate_over_raw_array)
    HM_TEST_RUN(test_can_expand_array_without_expand_func)
    HM_TEST_RUN(test_can_expand_array_with_expand_func)
    HM_TEST_RUN(test_can_set_array_item)
    HM_TEST_RUN(test_can_add_range_to_array)
    HM_TEST_RUN(test_can_add_range_to_array_with_new_count_exceeding_capacity_greater_than_growth_factor)
    HM_TEST_RUN(test_can_clear_array)
HM_TEST_SUITE_END()
