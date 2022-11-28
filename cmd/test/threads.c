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

#include "common.h"
#include <core/allocator.h>
#include <core/environment.h>
#include <core/string.h>
#include <threading/thread.h>

/* These tests rely on some timing, so sporadically they can fail on busy machines. */

#define THREAD_NAME "TestThread"
#define THREAD_JOIN_TIMEOUT (5*1000)

static void create_thread_and_allocator(hmThread* thread, hmAllocator* allocator, hmThreadStartFunc start_func, void* user_data)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    hmString name;
    err = hmCreateStringViewFromCString(THREAD_NAME, &name);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateThread(
        allocator,
        &name,
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
    return hmSleep(200);
}

static void test_can_start_sleep_and_join_thread()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &can_start_sleep_and_join_thread_func, HM_NULL);
    hmError err = hmThreadJoin(&thread, THREAD_JOIN_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    hmError exit_error = hmThreadGetExitError(&thread);
    HM_TEST_ASSERT_OK(exit_error);
    dispose_thread_and_allocator(&thread, &allocator);
}

static hmError returns_error_when_joining_self_thread_func(void* user_data)
{
    return hmThreadJoin((hmThread*)user_data, THREAD_JOIN_TIMEOUT);
}

static void test_returns_error_when_joining_self()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &returns_error_when_joining_self_thread_func, &thread);
    hmError err = hmThreadJoin(&thread, THREAD_JOIN_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    hmError exit_error = hmThreadGetExitError(&thread);
    HM_TEST_ASSERT(exit_error == HM_ERROR_INVALID_ARGUMENT);
    dispose_thread_and_allocator(&thread, &allocator);
}

static hmError threads_can_abort_thread_func(void* user_data)
{
    hmThread* thread = (hmThread*)user_data;
    while (hmThreadGetState(thread) != HM_THREAD_STATE_ABORT_REQUESTED) {
        hmError err = hmSleep(100);
        HM_TEST_ASSERT_OK(err);
    }
    return HM_OK;
}

static void test_threads_can_abort()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &threads_can_abort_thread_func, &thread);
    hmError err = hmSleep(200);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadAbort(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread, THREAD_JOIN_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    hmError exit_error = hmThreadGetExitError(&thread);
    HM_TEST_ASSERT_OK(exit_error);
    dispose_thread_and_allocator(&thread, &allocator);
}

static hmError can_join_too_late_thread_func(void* user_data)
{
    return HM_OK;
}

static void test_can_join_too_late()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &can_join_too_late_thread_func, HM_NULL);
    hmError err = hmSleep(300);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread, THREAD_JOIN_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    hmError exit_error = hmThreadGetExitError(&thread);
    HM_TEST_ASSERT_OK(exit_error);
    dispose_thread_and_allocator(&thread, &allocator);
}

static hmError threads_have_correct_statuses_thread_func(void* user_data)
{
    hmThread* thread = (hmThread*)user_data;
    HM_TEST_ASSERT(hmThreadGetState(thread) == HM_THREAD_STATE_RUNNING);
    return HM_OK;
}

static void test_threads_have_correct_statuses()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &threads_have_correct_statuses_thread_func, &thread);
    hmError err = hmThreadJoin(&thread, THREAD_JOIN_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmThreadGetState(&thread) == HM_THREAD_STATE_STOPPED);
    hmError exit_error = hmThreadGetExitError(&thread);
    HM_TEST_ASSERT_OK(exit_error);
    dispose_thread_and_allocator(&thread, &allocator);
}

static hmError can_dispose_thread_before_it_finishes_thread_func(void* user_data)
{
    return hmSleep(200);
}

static void test_can_dispose_thread_before_it_finishes()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &can_dispose_thread_before_it_finishes_thread_func, HM_NULL);
    hmError err = hmThreadDispose(&thread); /* Dispose immediately while the thread is running (sleeps for 200 ms). */
    HM_TEST_ASSERT_OK(err);
    err = hmSleep(400); /* Wait twice more time than the thread is running to make sure it stops running (to avoid memory leak report). */
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

static hmError can_retrieve_thread_name_thread_func(void* user_data)
{
    return HM_OK;
}

static void test_can_retrieve_thread_name()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &can_retrieve_thread_name_thread_func, HM_NULL);
    hmString thread_name;
    hmError err = hmThreadGetName(&thread, &thread_name);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(&thread_name, THREAD_NAME));
    err = hmStringDispose(&thread_name);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread, THREAD_JOIN_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    dispose_thread_and_allocator(&thread, &allocator);
}

static hmError thread_reports_processor_time_thread_func(void* user_data)
{
    hmThread* thread = (hmThread*)user_data;
    while (hmThreadGetState(thread) != HM_THREAD_STATE_ABORT_REQUESTED) {
        hmError err = hmSleep(100);
        HM_TEST_ASSERT_OK(err);
    }
    hm_millis processor_time = hmThreadGetProcessorTime(thread);
    HM_TEST_ASSERT(processor_time > 0);
    return HM_OK;
}

static void test_thread_reports_processor_time()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &thread_reports_processor_time_thread_func, &thread);
    hmError err = hmSleep(300);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadAbort(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread, THREAD_JOIN_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    dispose_thread_and_allocator(&thread, &allocator);
}

static hmError can_create_and_join_100_threads_thread_func(void* user_data)
{
    return hmSleep(10);
}

static void test_can_create_and_join_many_threads()
{
    #define THREAD_COUNT 50
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmString name;
    err = hmCreateStringViewFromCString(THREAD_NAME, &name);
    HM_TEST_ASSERT_OK(err);
    hmThread threads[THREAD_COUNT];
    for (hm_nint i = 0; i < THREAD_COUNT; i++) {
        err = hmCreateThread(
            &allocator,
            &name,
            &can_create_and_join_100_threads_thread_func,
            HM_NULL,
            &threads[i]
        );
        HM_TEST_ASSERT_OK(err);
    }
    for (hm_nint i = 0; i < THREAD_COUNT; i++) {
        err = hmThreadJoin(&threads[i], THREAD_JOIN_TIMEOUT);
        HM_TEST_ASSERT_OK(err);
    }
    for (hm_nint i = 0; i < THREAD_COUNT; i++) {
        err = hmThreadDispose(&threads[i]);
        HM_TEST_ASSERT_OK(err);
    }
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_can_sleep()
{
    hm_millis old_tick_count = hmGetTickCount();
    hmError err = hmSleep(1300);
    HM_TEST_ASSERT_OK(err);
    hm_millis time_diff = hmGetTickCount() - old_tick_count;
    HM_TEST_ASSERT(time_diff > 1250 && time_diff < 1600);
}

static hmError can_join_with_timeout_thread_func(void* user_data)
{
    return hmSleep(400);
}

static void test_can_join_with_timeout()
{
    hmAllocator allocator;
    hmThread thread;
    create_thread_and_allocator(&thread, &allocator, &can_join_with_timeout_thread_func, HM_NULL);
    hmError err = hmThreadJoin(&thread, 200);
    HM_TEST_ASSERT(err == HM_ERROR_TIMEOUT);
    err = hmThreadDispose(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmSleep(400); /* we need to wait for the thread to finish naturally or else we can get a memory leak report */
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

void test_threads()
{
    HM_TEST_SUITE_BEGIN("Threads");
        HM_TEST_RUN(test_can_start_sleep_and_join_thread);
        HM_TEST_RUN(test_returns_error_when_joining_self);
        HM_TEST_RUN(test_threads_can_abort);
        HM_TEST_RUN(test_can_join_too_late);
        HM_TEST_RUN(test_threads_have_correct_statuses);
        HM_TEST_RUN(test_can_dispose_thread_before_it_finishes);
        HM_TEST_RUN(test_can_retrieve_thread_name);
        HM_TEST_RUN(test_thread_reports_processor_time);
        HM_TEST_RUN(test_can_create_and_join_many_threads);
        HM_TEST_RUN(test_can_sleep);
        HM_TEST_RUN(test_can_join_with_timeout);
    HM_TEST_SUITE_END();
}
