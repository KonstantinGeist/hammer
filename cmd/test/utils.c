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
#include <core/utils.h>

static void test_can_align_size()
{
    HM_TEST_ASSERT(hmAlignSize(13) == 16);
    HM_TEST_ASSERT(hmAlignSize(4) == 16);
    HM_TEST_ASSERT(hmAlignSize(16) == 16);
    HM_TEST_ASSERT(hmAlignSize(17) == 32);
    HM_TEST_ASSERT(hmAlignSize(0) == 0);
}

void test_utils()
{
    HM_TEST_SUITE_BEGIN("Utils");
        HM_TEST_RUN(test_can_align_size);
    HM_TEST_SUITE_END();
}
