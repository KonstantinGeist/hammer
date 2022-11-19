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
#include <core/environment.h>
#include <threading/thread.h>

static void test_tick_count_grows_monotonically()
{
    hm_nint first_tick_count = hmGetTickCount();
    hmError err = hmSleep(100);
    HM_TEST_ASSERT_OK(err);
    hm_nint second_tick_count = hmGetTickCount();
    HM_TEST_ASSERT(second_tick_count > first_tick_count);
}

void test_environment()
{
    HM_TEST_LOG("Environment...");
    test_tick_count_grows_monotonically();
}
