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
#include <threading/thread.h>
#include <threading/waitobject.h>

/* These tests rely on some timing, so sporadically they can fail on busy machines. */

#define TEST_WAIT_PULSE_ITERATION_COUNT 3

static void test_wait_object_can_timeout()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmWaitObject wait_object;
    err = hmCreateWaitObject(&allocator, &wait_object);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitObjectWait(&wait_object, 250);
    HM_TEST_ASSERT(err == HM_ERROR_TIMEOUT); /* no one to pulse it, so naturally it times out */
    err = hmWaitObjectDispose(&wait_object);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

typedef struct {
    hmWaitObject   wait_object;
volatile
    hm_atomic_nint result;
} shared_thread_context;

static hmError producer_thread_proc(void* user_data)
{
    shared_thread_context* context = (shared_thread_context*)user_data;
    hmError err = hmSleep(300); /* Wait, maybe the consumer is still launching. */
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmAtomicLoad(&context->result) == 0); /* Before we produce anything, it should be 0. */
    for (hm_nint i = 0; i < TEST_WAIT_PULSE_ITERATION_COUNT; i++) {
        hmError err = hmWaitObjectPulse(&context->wait_object);
        HM_TEST_ASSERT_OK(err);
        err = hmSleep(200);
        HM_TEST_ASSERT_OK(err);
    }
    return HM_OK;
}

static hmError consumer_thread_proc(void* user_data)
{
    shared_thread_context* context = (shared_thread_context*)user_data;
    hmError err = hmWaitObjectWait(&context->wait_object, 100); /* Should timeout, because the producer is waiting for 300ms. */
    HM_TEST_ASSERT(err == HM_ERROR_TIMEOUT);
    while (HM_TRUE) {
        hmError err = hmWaitObjectWait(&context->wait_object, 10000); /* A large value for a test (10 seconds) but must be preempted with a Pulse() */
        if (err == HM_OK) {
            (void)hmAtomicIncrement(&context->result);
        } else if (err == HM_ERROR_TIMEOUT) { /* Timeout is not an error, just retry. */
            continue;
        }
        HM_TEST_ASSERT_OK(err);
        if (hmAtomicLoad(&context->result) == TEST_WAIT_PULSE_ITERATION_COUNT) {
            break;
        }
    }
    return HM_OK;
}

static void test_can_wait_and_pulse_with_wait_objects()
{
    #define TEST_THREAD_COUNT 2
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    shared_thread_context context;
    hmAtomicStore(&context.result, 0);
    err = hmCreateWaitObject(&allocator, &context.wait_object);
    HM_TEST_ASSERT_OK(err);
    hmThread threads[TEST_THREAD_COUNT];
    for (hm_nint i = 0; i < TEST_THREAD_COUNT; i++) {
        err = hmCreateThread(
            &allocator,
            HM_NULL,
            i % 2 == 0 ? &producer_thread_proc : &consumer_thread_proc,
            &context,
            &threads[i]
        );
        HM_TEST_ASSERT_OK(err);
    }
    for (hm_nint i = 0; i < TEST_THREAD_COUNT; i++) {
        err = hmThreadJoin(&threads[i]);
        HM_TEST_ASSERT_OK(err);
    }
    for (hm_nint i = 0; i < TEST_THREAD_COUNT; i++) {
        err = hmThreadDispose(&threads[i]);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_ASSERT(hmAtomicLoad(&context.result) == TEST_WAIT_PULSE_ITERATION_COUNT);
    err = hmWaitObjectDispose(&context.wait_object);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

void test_wait_objects()
{
    HM_TEST_LOG("WaitObjects...");
    test_wait_object_can_timeout();
    test_can_wait_and_pulse_with_wait_objects();
}
