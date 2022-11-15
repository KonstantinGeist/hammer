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

#include <stdio.h>

static hmError thread_func(void* user_data)
{
    return hmSleep(500);
}

static void test_can_start_sleep_and_join_thread()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmThread thread;
    hmString name;
    err = hmCreateStringViewFromCString("TestThread", &name);
    HM_TEST_ASSERT_OK(err);
    hmThreadProperties thread_properties;
    thread_properties.name = (struct _hmString*)&name; // TODO don't force to cast
    err = hmCreateThread(
        &allocator,
        thread_properties,
        &thread_func,
        &thread,
        &thread
    );
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread);
    HM_TEST_ASSERT_OK(err);
    hmError exit_error = hmThreadGetExitError(&thread);
    HM_TEST_ASSERT_OK(exit_error);
    err = hmThreadDispose(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

void test_threads()
{
    test_can_start_sleep_and_join_thread();
}
