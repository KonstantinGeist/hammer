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
#include <threading/mutex.h>

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

void test_mutexes()
{
    test_can_create_lock_unlock_dispose_mutex_in_general();
}
