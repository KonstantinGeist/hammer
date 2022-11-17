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
#include <threading/waitobject.h>

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

void test_wait_objects()
{
    HM_TEST_LOG("WaitObject...");
    test_wait_object_can_timeout();
}
