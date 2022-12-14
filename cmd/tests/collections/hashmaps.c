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
#include <collections/hashmap.h>
#include <core/primitives.h>
#include <core/string.h>

#define ITERATION_COUNT 1000
#define HASH_SALT 666

typedef struct {
    int x, y;
} Point;

static void create_integer_hash_map_and_allocator(hmHashMap* hash_map, hmAllocator* allocator)
{
    HM_TEST_INIT_ALLOC(allocator);
    HM_TEST_TRACK_OOM(allocator, HM_FALSE);
    hmError err = hmCreateHashMap(
        allocator,
        &hmNintHashFunc,
        &hmNintEqualsFunc,
        HM_NULL, /* key_dispose_func */
        HM_NULL, /* value_dispose_func */
        sizeof(hm_nint),
        sizeof(hm_nint),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        HASH_SALT,
        hash_map
    );
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(allocator, HM_TRUE);
}

static void create_point_hash_map_and_allocator(hmHashMap* hash_map, hmAllocator* allocator)
{
    HM_TEST_INIT_ALLOC(allocator);
    HM_TEST_TRACK_OOM(allocator, HM_FALSE);
    hmError err = hmCreateHashMap(
        allocator,
        HM_NULL, /* hash_func */
        HM_NULL, /* equals_func */
        HM_NULL, /* key_dispose_func */
        HM_NULL, /* value_dispose_func */
        sizeof(Point),
        sizeof(hm_nint),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        HASH_SALT,
        hash_map
    );
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(allocator, HM_TRUE);
}

static void create_string_hash_map_and_allocator_with_dispose_func(hmHashMap* hash_map, hmAllocator* allocator)
{
    HM_TEST_INIT_ALLOC(allocator);
    HM_TEST_TRACK_OOM(allocator, HM_FALSE);
    hmError err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmStringDisposeFunc, /* value_dispose_func */
        sizeof(hmString),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        HASH_SALT,
        hash_map
    );
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(allocator, HM_TRUE);
}

static void dispose_hash_map_and_allocator(hmHashMap* hash_map, hmAllocator* allocator)
{
    hmError err = hmHashMapDispose(hash_map);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(allocator);
}

static void test_can_create_and_dispose_hash_map()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_integer_hash_map_and_allocator(&hash_map, &allocator);
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

static void test_can_put_and_get_integers_from_hash_map()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_integer_hash_map_and_allocator(&hash_map, &allocator);
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) { /* also tests rehashing */
        hm_nint value = i*2;
        hmError err = hmHashMapPut(&hash_map, &i, &value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        hm_nint retrieved_value;
        err = hmHashMapGet(&hash_map, &i, &retrieved_value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(value == retrieved_value);
    }
HM_TEST_ON_FINALIZE
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

static void test_can_remove_integers_from_hash_map()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_integer_hash_map_and_allocator(&hash_map, &allocator);
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) {
        hm_nint value = i*2;
        hmError err = hmHashMapPut(&hash_map, &i, &value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) { /* removes all non-odd elements */
        if (i % 2 == 0) {
            hm_bool removed;
            hmError err = hmHashMapRemove(&hash_map, &i, &removed);
            HM_TEST_ASSERT_OK_OR_OOM(err);
            HM_TEST_ASSERT(removed);
        }
    }
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) {
        hm_nint retrieved_value;
        hmError err = hmHashMapGet(&hash_map, &i, &retrieved_value);
        if (i % 2 == 0) {
            HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
        } else {
            HM_TEST_ASSERT_OK_OR_OOM(err);
        }
    }
HM_TEST_ON_FINALIZE
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

static void test_hash_map_returns_error_on_non_existing_key()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_integer_hash_map_and_allocator(&hash_map, &allocator);
    hm_nint value = 7;
    hmError err = hmHashMapPut(&hash_map, &value, &value);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hm_nint non_existing_key = 8;
    hm_nint retrieved_value;
    err = hmHashMapGet(&hash_map, &non_existing_key, &retrieved_value);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
HM_TEST_ON_FINALIZE
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

static void test_hash_map_reports_nothing_was_removed()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_integer_hash_map_and_allocator(&hash_map, &allocator);
    hm_nint key = 10;
    hm_bool removed = HM_TRUE;
    hmError err = hmHashMapRemove(&hash_map, &key, &removed);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(!removed);
HM_TEST_ON_FINALIZE
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

static void test_hash_map_reports_correct_count()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_integer_hash_map_and_allocator(&hash_map, &allocator);
    HM_TEST_ASSERT(hmHashMapGetCount(&hash_map) == 0);
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) {
        hm_nint value = i*2;
        hmError err = hmHashMapPut(&hash_map, &i, &value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    HM_TEST_ASSERT(hmHashMapGetCount(&hash_map) == ITERATION_COUNT);
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) { /* removes all non-odd elements */
        if (i % 2 == 0) {
            hm_bool removed;
            hmError err = hmHashMapRemove(&hash_map, &i, &removed);
            HM_TEST_ASSERT_OK_OR_OOM(err);
            HM_TEST_ASSERT(removed);
        }
    }
    HM_TEST_ASSERT(hmHashMapGetCount(&hash_map) == ITERATION_COUNT/2);
HM_TEST_ON_FINALIZE
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

static void test_can_put_remove_and_get_strings_from_hash_map_with_dispose_func()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_string_hash_map_and_allocator_with_dispose_func(&hash_map, &allocator);
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) {
        hmString str_key, str_value;
        hmError err = hmInt32ToString(&allocator, (hm_int32)i, &str_key);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        err = hmInt32ToString(&allocator, (hm_int32)i*2, &str_value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        err = hmHashMapPut(&hash_map, &str_key, &str_value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) { /* removes all non-odd elements */
        if (i % 2 == 0) {
            hmString str_key;
            hmError err = hmInt32ToString(&allocator, (hm_int32)i, &str_key);
            HM_TEST_ASSERT_OK_OR_OOM(err);
            hm_bool removed;
            err = hmHashMapRemove(&hash_map, &str_key, &removed);
            HM_TEST_ASSERT_OK_OR_OOM(err);
            HM_TEST_ASSERT(removed);
            err = hmStringDispose(&str_key);
            HM_TEST_ASSERT_OK_OR_OOM(err);
        }
    }
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) {
        hmString str_key;
        hmError err = hmInt32ToString(&allocator, (hm_int32)i, &str_key);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        hmString* retrieved_value;
        err = hmHashMapGet(&hash_map, &str_key, &retrieved_value);
        if (i % 2 == 0) {
            HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
        } else {
            HM_TEST_ASSERT_OK_OR_OOM(err);
        }
        err = hmStringDispose(&str_key);
        HM_TEST_ASSERT_OK_OR_OOM(err);
    }
HM_TEST_ON_FINALIZE
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

static void test_can_put_remove_and_get_strings_from_hash_map_without_hash_equals_funcs()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_point_hash_map_and_allocator(&hash_map, &allocator);
    for (hm_nint i = 0; i < ITERATION_COUNT; i++) { /* also tests rehashing */
        Point value;
        value.x = i*20;
        value.y = i*30;
        hmError err = hmHashMapPut(&hash_map, &value, &i);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        hm_nint retrieved_value;
        err = hmHashMapGet(&hash_map, &value, &retrieved_value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(i == retrieved_value);
    }
HM_TEST_ON_FINALIZE
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

static void test_hashmap_can_get_value_by_ref()
{
    hmAllocator allocator;
    hmHashMap hash_map;
    create_integer_hash_map_and_allocator(&hash_map, &allocator);
    hm_nint key = 10;
    hm_nint value = 20;
    hmError err = hmHashMapPut(&hash_map, &key, &value);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    void* retrieved_value_by_ref;
    err = hmHashMapGetRef(&hash_map, &key, &retrieved_value_by_ref);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(retrieved_value_by_ref != HM_NULL);
    HM_TEST_ASSERT(*((hm_nint*)retrieved_value_by_ref) == value);
    *((hm_nint*)retrieved_value_by_ref) = 13;
    hm_nint retrieved_value;
    err = hmHashMapGet(&hash_map, &key, &retrieved_value);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(retrieved_value == 13);
HM_TEST_ON_FINALIZE
    dispose_hash_map_and_allocator(&hash_map, &allocator);
}

HM_TEST_SUITE_BEGIN(hashmaps)
    HM_TEST_RUN(test_can_create_and_dispose_hash_map)
    HM_TEST_RUN(test_can_put_and_get_integers_from_hash_map)
    HM_TEST_RUN(test_can_remove_integers_from_hash_map)
    HM_TEST_RUN(test_hash_map_returns_error_on_non_existing_key)
    HM_TEST_RUN(test_hash_map_reports_nothing_was_removed)
    HM_TEST_RUN(test_hash_map_reports_correct_count)
    HM_TEST_RUN_WITHOUT_OOM(test_can_put_remove_and_get_strings_from_hash_map_with_dispose_func) /* without OOM: takes too much time */
    HM_TEST_RUN(test_can_put_remove_and_get_strings_from_hash_map_without_hash_equals_funcs)
    HM_TEST_RUN(test_hashmap_can_get_value_by_ref)
HM_TEST_SUITE_END()
