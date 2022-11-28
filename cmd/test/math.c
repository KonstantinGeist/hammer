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
#include <core/math.h>

static void test_detects_nint_overflow_when_adding()
{
    hm_nint result = 0;
    hmError err = hmAddNint(HM_NINT_MAX - 10, 20, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddNint(20, 30, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 50);
    err = hmAddNint(0, 10, &result);
    HM_TEST_ASSERT(result == 10);
    err = hmAddNint(10, 0, &result);
    HM_TEST_ASSERT(result == 10);
    err = hmAddNint(HM_NINT_MAX, 5, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddNint(HM_NINT_MAX, HM_NINT_MAX, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddNint(HM_NINT_MAX, 0, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == HM_NINT_MAX);
    err = hmAddNint(0, HM_NINT_MAX, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == HM_NINT_MAX);
}

void test_math()
{
    HM_TEST_SUITE_BEGIN("Math");
        HM_TEST_RUN(test_detects_nint_overflow_when_adding);
    HM_TEST_SUITE_END();
}
