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
    hm_millis first_tick_count = hmGetTickCount();
    hmError err = hmSleep(100);
    HM_TEST_ASSERT_OK(err);
    hm_millis second_tick_count = hmGetTickCount();
    HM_TEST_ASSERT(second_tick_count > first_tick_count);
}

static void test_can_get_processor_count()
{
    hm_nint processor_count = hmGetProcessorCount();
    HM_TEST_ASSERT(processor_count > 0);
}

void test_environment()
{
    HM_TEST_SUITE_BEGIN("Environment");
        HM_TEST_RUN(test_tick_count_grows_monotonically);
        HM_TEST_RUN(test_can_get_processor_count);
    HM_TEST_SUITE_END();
}
