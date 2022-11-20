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
#include <core/allocator.h>
#include <core/primitives.h>
#include <collections/hashmap.h>
#include <threading/mutex.h>
#include <threading/thread.h>

#define TEST_THREAD_JOIN_TIMEOUT (5*1000)

static void create_mutex_and_allocator(hmMutex* mutex, hmAllocator* allocator)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateMutex(
        allocator,
        mutex
    );
    HM_TEST_ASSERT_OK(err);
}

static void dispose_mutex_and_allocator(hmMutex* mutex, hmAllocator* allocator)
{
    hmError err = hmMutexDispose(mutex);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_can_create_lock_unlock_dispose_mutex_in_general()
{
    hmMutex mutex;
    hmAllocator allocator;
    create_mutex_and_allocator(&mutex, &allocator);
    hmError err = hmMutexLock(&mutex);
    HM_TEST_ASSERT_OK(err);
    err = hmMutexLock(&mutex); /* checking if can lock recursively (would return an error or block infinitely otherwise) */
    HM_TEST_ASSERT_OK(err);
    err = hmMutexUnlock(&mutex);
    HM_TEST_ASSERT_OK(err);
    err = hmMutexUnlock(&mutex);
    HM_TEST_ASSERT_OK(err);
    dispose_mutex_and_allocator(&mutex, &allocator);
}

typedef struct {
    hmAllocator allocator;
    hmMutex     mutex;
    hmHashMap   hash_map;
} shared_hash_map_and_mutex_context;

static hmError mutexes_protect_from_data_corruption_thread_func(void* user_data)
{
    shared_hash_map_and_mutex_context* context = (shared_hash_map_and_mutex_context*)user_data;
    hmError err = hmMutexLock(&context->mutex);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateHashMap(
        &context->allocator,
        &hmNintHashFunc,
        &hmNintEqualsFunc,
        HM_NULL,      /* key_dispose_func */
        HM_NULL,      /* value_dispose_func */
        sizeof(hm_nint),
        sizeof(hm_nint),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        &context->hash_map
    );
    HM_TEST_ASSERT_OK(err);
    err = hmSleep(100);
    HM_TEST_ASSERT_OK(err);
    err = hmHashMapDispose(&context->hash_map);
    HM_TEST_ASSERT_OK(err);
    err = hmMutexUnlock(&context->mutex);
    HM_TEST_ASSERT_OK(err);
    return HM_OK;
}

static void test_mutexes_protect_from_data_corruption()
{
    shared_hash_map_and_mutex_context context;
    hmError err = hmCreateSystemAllocator(&context.allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateMutex(
        &context.allocator,
        &context.mutex
    );
    HM_TEST_ASSERT_OK(err);
    #define TEST_THREAD_COUNT 20
    hmThread threads[TEST_THREAD_COUNT];
    for (hm_nint i = 0; i < TEST_THREAD_COUNT; i++) {
        err = hmCreateThread(
            &context.allocator,
            HM_NULL,
            &mutexes_protect_from_data_corruption_thread_func,
            &context,
            &threads[i]
        );
        HM_TEST_ASSERT_OK(err);
    }
    for (hm_nint i = 0; i < TEST_THREAD_COUNT; i++) {
        err = hmThreadJoin(&threads[i], TEST_THREAD_JOIN_TIMEOUT);
        HM_TEST_ASSERT_OK(err);
    }
    for (hm_nint i = 0; i < TEST_THREAD_COUNT; i++) {
        err = hmThreadDispose(&threads[i]);
        HM_TEST_ASSERT_OK(err);
    }
    err = hmMutexDispose(&context.mutex);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&context.allocator);
    HM_TEST_ASSERT_OK(err);
}

void test_mutexes()
{
    HM_TEST_SUITE_BEGIN("Mutexes");
        HM_TEST_RUN(test_can_create_lock_unlock_dispose_mutex_in_general);
        HM_TEST_RUN(test_mutexes_protect_from_data_corruption);
    HM_TEST_SUITE_END();
}
