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
#include "../src/hashmap.h"
#include "../src/allocator.h"
#include "../src/primitives.h"

static void create_int_hash_map_and_allocator(hmHashMap* hash_map, hmAllocator* allocator)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateHashMap(
        allocator,
        &hmNintHashFunc,
        &hmNintEqualsFunc,
        HM_NULL, // dispose_func
        sizeof(hm_nint),
        sizeof(hm_nint),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        hash_map
    );
    HM_TEST_ASSERT_OK(err);
}

static void dispose_memory_reader_and_allocator(hmHashMap* hash_map, hmAllocator* allocator)
{
    hmError err = hmHashMapDispose(hash_map);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_can_create_and_dispose_hash_map()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_int_hash_map_and_allocator(&hash_map, &allocator);
    dispose_memory_reader_and_allocator(&hash_map, &allocator);
}

static void test_can_put_and_get_integers_from_hash_map()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_int_hash_map_and_allocator(&hash_map, &allocator);
    for (hm_nint i = 0; i < 1000; i++) { // also tests rehashing
        hm_nint value = i*2;
        hmError err = hmHashMapPut(&hash_map, &i, &value);
        HM_TEST_ASSERT_OK(err);
        hm_nint retrieved_value;
        err = hmHashMapGet(&hash_map, &i, &retrieved_value);
        HM_TEST_ASSERT_OK(err);
        HM_TEST_ASSERT(value == retrieved_value);
    }
    dispose_memory_reader_and_allocator(&hash_map, &allocator);
}

void test_hashmaps()
{
    test_can_create_and_dispose_hash_map();
    test_can_put_and_get_integers_from_hash_map();
}
