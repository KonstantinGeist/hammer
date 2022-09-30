// *****************************************************************************
//
//  Copyright (c) Konstantin Geist. All rights reserved.
//
//  The use and distribution terms for this software are contained in the file
//  named License.txt, which can be found in the root of this distribution.
//  By using this software in any fashion, you are agreeing to be bound by the
//  terms of this license.
//
//  You must not remove this notice, or any other, from this software.
//
// *****************************************************************************

#include "common.h"
#include "../src/allocator.h"
#include "../src/array.h"

#define TEST_ARRAY_CAPACITY 4

typedef struct {
    hm_nint x;
    hm_nint y;
} testItem;

static hm_nint item_dispose_sum = 0;

static void create_array_and_allocator(hmArray* array, hmAllocator* allocator, hmDisposeFunc item_dispose_func)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateArray(allocator, sizeof(testItem), TEST_ARRAY_CAPACITY, item_dispose_func, array);
    HM_TEST_ASSERT_OK(err);
}

static void dispose_array_and_allocator(hmArray* array, hmAllocator* allocator)
{
    hmError err = hmArrayDispose(array);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_array_can_create_add_get_dispose_without_item_dispose_func()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, HM_NULL);
    for (hm_nint i = 0; i < TEST_ARRAY_CAPACITY+5; i++) { // note: also checks reallocations
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK(err);
        testItem retrieved_item;
        err = hmArrayGet(&array, i, &retrieved_item);
        HM_TEST_ASSERT_OK(err);
        HM_TEST_ASSERT(test_item.x == retrieved_item.x);
        HM_TEST_ASSERT(test_item.y == retrieved_item.y);
    }
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
    for (hm_nint i = 0; i < TEST_ARRAY_CAPACITY*2+1; i++) { // note: also checks reallocations
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        item_dispose_sum_control += test_item.x+test_item.y;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK(err);
    }
    dispose_array_and_allocator(&array, &allocator);
    HM_TEST_ASSERT(item_dispose_sum == item_dispose_sum_control);
}

static void test_returns_error_if_out_of_range()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    testItem test_item;
    test_item.x = 10;
    test_item.y = 20;
    hmError err = hmArrayAdd(&array, &test_item);
    HM_TEST_ASSERT_OK(err);
    err = hmArrayGet(&array, 2, &test_item);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_RANGE);
    dispose_array_and_allocator(&array, &allocator);
}

void test_arrays()
{
    test_array_can_create_add_get_dispose_without_item_dispose_func();
    test_array_can_create_add_get_dispose_with_item_dispose_func();
    test_returns_error_if_out_of_range();
}
