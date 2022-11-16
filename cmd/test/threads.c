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
#include <core/string.h>
#include <threading/thread.h>

#define TEST_THREAD_NAME "TestThread"

static void create_thread_and_allocator(hmThread* thread, hmAllocator* allocator, hmThreadStartFunc start_func, void* user_data)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    hmString name;
    err = hmCreateStringViewFromCString(TEST_THREAD_NAME, &name);
    HM_TEST_ASSERT_OK(err);
    hmThreadProperties thread_properties;
    thread_properties.name = &name;
    err = hmCreateThread(
        allocator,
        thread_properties,
        start_func,
        user_data,
        thread
    );
    HM_TEST_ASSERT_OK(err);
}

static void dispose_thread_and_allocator(hmThread* thread, hmAllocator* allocator)
{
    hmError err = hmThreadDispose(thread);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static hmError can_start_sleep_and_join_thread_func(void* user_data)
{
    return hmSleep(500);
}

static void test_can_start_sleep_and_join_thread()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &can_start_sleep_and_join_thread_func, HM_NULL);
    hmError err = hmThreadJoin(&thread);
    HM_TEST_ASSERT_OK(err);
    hmError exit_error = hmThreadGetExitError(&thread);
    HM_TEST_ASSERT_OK(exit_error);
    dispose_thread_and_allocator(&thread, &allocator);
}

static hmError returns_error_when_joining_self_thread_func(void* user_data)
{
    return hmThreadJoin((hmThread*)user_data);
}

static void test_returns_error_when_joining_self()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &returns_error_when_joining_self_thread_func, &thread);
    hmError err = hmThreadJoin(&thread);
    HM_TEST_ASSERT_OK(err);
    hmError exit_error = hmThreadGetExitError(&thread);
    HM_TEST_ASSERT(exit_error == HM_ERROR_INVALID_ARGUMENT);
    dispose_thread_and_allocator(&thread, &allocator);
}

void test_threads()
{
    test_can_start_sleep_and_join_thread();
    test_returns_error_when_joining_self();
}
