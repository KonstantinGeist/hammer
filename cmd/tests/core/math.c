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

#include "../common.h"
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
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 10);
    err = hmAddNint(10, 0, &result);
    HM_TEST_ASSERT_OK(err);
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

static void test_detects_nint_overflow_when_adding_3_nints()
{
    hm_nint result = 0;
    hmError err = hmAddNint3(2, 3, 4, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 9);
    err = hmAddNint3(HM_NINT_MAX - 10, 2, 2, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == HM_NINT_MAX - 6);
    err = hmAddNint3(HM_NINT_MAX - 10, 2, 9, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddNint3(HM_NINT_MAX - 10, 9, 2, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddNint3(9, HM_NINT_MAX - 10, 2, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddNint3(9, 2, HM_NINT_MAX - 10, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddNint3(2, HM_NINT_MAX - 10, 2, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == HM_NINT_MAX - 6);
    err = hmAddNint3(2, 2, HM_NINT_MAX - 10, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == HM_NINT_MAX - 6);
}

static void test_detects_nint_overflow_when_multiplying()
{
    hm_nint result = 0;
    hmError err = hmMulNint(2, 3, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 6);
    err = hmMulNint(HM_NINT_MAX - 1, 2, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmMulNint(HM_NINT_MAX - 1, 0, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 0);
    err = hmMulNint(0, HM_NINT_MAX - 1, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 0);
    err = hmMulNint(HM_NINT_MAX, HM_NINT_MAX, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmMulNint(HM_NINT_MAX/2, 3, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
}

static void test_detects_nint_overflow_when_adding_and_multiplying()
{
    hm_nint result = 0;
    hmError err = hmAddMulNint(2, 3, 4, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 14);
    err = hmAddMulNint(HM_NINT_MAX - 1, 3, 4, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddMulNint(0, HM_NINT_MAX, 4, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddMulNint(4, 1, HM_NINT_MAX - 2, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddMulNint(4, 0, HM_NINT_MAX - 2, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 4);
    err = hmAddMulNint(7, HM_NINT_MAX - 2, 0, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 7);
}

static void test_detects_millis_overflow_when_adding()
{
    hm_millis result = 0;
    hmError err = hmAddMillis(HM_MILLIS_MAX - 10, 20, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddMillis(20, 30, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 50);
    err = hmAddMillis(0, 10, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 10);
    err = hmAddMillis(10, 0, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 10);
    err = hmAddMillis(HM_MILLIS_MAX, 5, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddMillis(HM_MILLIS_MAX, HM_MILLIS_MAX, &result);
    HM_TEST_ASSERT(err == HM_ERROR_OVERFLOW);
    err = hmAddMillis(HM_MILLIS_MAX, 0, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == HM_MILLIS_MAX);
    err = hmAddMillis(0, HM_MILLIS_MAX, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == HM_MILLIS_MAX);
}

static void test_detects_underflow_when_subtracting()
{
    hm_nint result = 0;
    hmError err = hmSubNint(3, 1, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 2);
    err = hmSubNint(1, 3, &result);
    HM_TEST_ASSERT(err == HM_ERROR_UNDERFLOW);
}

static void test_abs()
{
    hm_int32 result = 0;
    hmError err = hmAbsInt32(5, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 5);
    err = hmAbsInt32(-5, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == 5);
    err = hmAbsInt32(HM_INT32_MAX, &result);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(result == HM_INT32_MAX);
    err = hmAbsInt32(HM_INT32_MIN, &result);
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_ARGUMENT);
}

HM_TEST_SUITE_BEGIN(math)
    HM_TEST_RUN_WITHOUT_OOM(test_detects_nint_overflow_when_adding)
    HM_TEST_RUN_WITHOUT_OOM(test_detects_nint_overflow_when_multiplying)
    HM_TEST_RUN_WITHOUT_OOM(test_detects_nint_overflow_when_adding_3_nints)
    HM_TEST_RUN_WITHOUT_OOM(test_detects_nint_overflow_when_adding_and_multiplying)
    HM_TEST_RUN_WITHOUT_OOM(test_detects_millis_overflow_when_adding)
    HM_TEST_RUN_WITHOUT_OOM(test_detects_underflow_when_subtracting)
    HM_TEST_RUN_WITHOUT_OOM(test_abs)
HM_TEST_SUITE_END()
