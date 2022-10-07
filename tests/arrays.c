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

#define TEST_ARRAY_CAPACITY     4
#define TEST_ARRAY_EXPAND_COUNT 100 // big enough to also test reallocation

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

static void test_returns_error_if_get_out_of_range()
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
    for (hm_nint i = 0; i < TEST_ARRAY_CAPACITY; i++) {
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK(err);
    }
    testItem* raw_items = hmArrayRaw(&array, testItem);
    for (hm_nint i = 0; i < hmArrayCount(&array); i++) {
        testItem control_item;
        control_item.x = i*10;
        control_item.y = i*20;
        testItem retrieved_item = raw_items[i];
        HM_TEST_ASSERT(control_item.x == retrieved_item.x);
        HM_TEST_ASSERT(control_item.y == retrieved_item.y);
    }
    dispose_array_and_allocator(&array, &allocator);
}

static void test_can_expand_array_without_expand_func()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    for (hm_nint i = 0; i < TEST_ARRAY_CAPACITY; i++) {
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK(err);
    }
    hmError err = hmArrayExpand(&array, TEST_ARRAY_EXPAND_COUNT, HM_NULL, HM_NULL);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmArrayCount(&array) == TEST_ARRAY_CAPACITY+TEST_ARRAY_EXPAND_COUNT);
    testItem* test_item = hmArrayRaw(&array, testItem)+TEST_ARRAY_CAPACITY;
    for (hm_nint i = 0; i < TEST_ARRAY_EXPAND_COUNT; i++) {
        HM_TEST_ASSERT(test_item->x == 0);
        HM_TEST_ASSERT(test_item->y == 0);
        test_item++;
    }
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
    for (hm_nint i = 0; i < TEST_ARRAY_CAPACITY; i++) {
        testItem test_item;
        test_item.x = i*10;
        test_item.y = i*20;
        hmError err = hmArrayAdd(&array, &test_item);
        HM_TEST_ASSERT_OK(err);
    }
    hm_nint base_int = 666;
    hmError err = hmArrayExpand(&array, TEST_ARRAY_EXPAND_COUNT, &array_expand_func, &base_int);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmArrayCount(&array) == TEST_ARRAY_CAPACITY+TEST_ARRAY_EXPAND_COUNT);
    testItem* test_item = hmArrayRaw(&array, testItem)+TEST_ARRAY_CAPACITY;
    for (hm_nint i = 0; i < TEST_ARRAY_EXPAND_COUNT; i++) {
        HM_TEST_ASSERT(test_item->x == base_int+(i+TEST_ARRAY_CAPACITY)*10);
        HM_TEST_ASSERT(test_item->y == base_int+(i+TEST_ARRAY_CAPACITY)*20);
        test_item++;
    }
    dispose_array_and_allocator(&array, &allocator);
}

static void test_can_set_array_item()
{
    hmAllocator allocator;
    hmArray array;
    create_array_and_allocator(&array, &allocator, &item_dispose_func);
    hmError err = hmArrayExpand(&array, 4, HM_NULL, HM_NULL);
    HM_TEST_ASSERT_OK(err);
    testItem test_item;
    test_item.x = 13;
    test_item.y = 666;
    err = hmArraySet(&array, 2, &test_item);
    HM_TEST_ASSERT_OK(err);
    testItem retrieved_item;
    err = hmArrayGet(&array, 2, &retrieved_item);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(test_item.x == retrieved_item.x);
    HM_TEST_ASSERT(test_item.y == retrieved_item.y);
    dispose_array_and_allocator(&array, &allocator);
}

void test_arrays()
{
    test_array_can_create_add_get_dispose_without_item_dispose_func();
    test_array_can_create_add_get_dispose_with_item_dispose_func();
    test_returns_error_if_get_out_of_range();
    test_returns_error_if_set_out_of_range();
    test_can_iterate_over_raw_array();
    test_can_expand_array_without_expand_func();
    test_can_expand_array_with_expand_func();
    test_can_set_array_item();
}
