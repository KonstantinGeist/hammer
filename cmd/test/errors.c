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

static void test_can_combine_errors()
{
    hmError err = hmCombineErrors(HM_OK, HM_ERROR_OUT_OF_MEMORY);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_MEMORY);
    err = hmCombineErrors(HM_ERROR_OUT_OF_MEMORY, HM_OK);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_MEMORY);
    err = hmCombineErrors(HM_OK, HM_OK);
    HM_TEST_ASSERT(err == HM_OK);
    err = hmCombineErrors(HM_ERROR_OUT_OF_MEMORY, HM_ERROR_NOT_FOUND);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_MEMORY);
}

void test_errors()
{
    HM_TEST_SUITE_BEGIN("Errors");
        HM_TEST_RUN(test_can_combine_errors);
    HM_TEST_SUITE_END();
}
