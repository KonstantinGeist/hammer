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

static void test_can_merge_errors()
{
    hmError err = hmMergeErrors(HM_OK, HM_ERROR_OUT_OF_MEMORY);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_MEMORY);
    err = hmMergeErrors(HM_ERROR_OUT_OF_MEMORY, HM_OK);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_MEMORY);
    err = hmMergeErrors(HM_OK, HM_OK);
    HM_TEST_ASSERT(err == HM_OK);
    err = hmMergeErrors(HM_ERROR_OUT_OF_MEMORY, HM_ERROR_NOT_FOUND);
    HM_TEST_ASSERT(err == HM_ERROR_OUT_OF_MEMORY);
}

HM_TEST_SUITE_BEGIN(errors)
    HM_TEST_RUN_WITHOUT_OOM(test_can_merge_errors)
HM_TEST_SUITE_END()
