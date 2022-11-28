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
#include <core/random.h>

static void test_random_generates_int_sequence()
{
    hm_int32 random_values[10] = {0};
    hmRandom random;
    hmError err = hmCreateRandom(666, &random);
    HM_TEST_ASSERT_OK(err);
    for (hm_nint i = 0; i < 10; i++) {
        random_values[i] = hmRandomGetNextInt(&random);
    }
    err = hmRandomDispose(&random);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(random_values[0] == 465257956); /* precomputed */
    HM_TEST_ASSERT(random_values[1] == 1741838509);
    HM_TEST_ASSERT(random_values[2] == 965439257);
    HM_TEST_ASSERT(random_values[3] == 1180762009);
    HM_TEST_ASSERT(random_values[4] == 689623435);
    HM_TEST_ASSERT(random_values[5] == 2056146873);
    HM_TEST_ASSERT(random_values[6] == 133547913);
    HM_TEST_ASSERT(random_values[7] == 2112289963);
    HM_TEST_ASSERT(random_values[8] == 1592106521);
    HM_TEST_ASSERT(random_values[9] == 1329609269);
}

static void test_random_generates_float_sequence()
{
    hmRandom random;
    hmError err = hmCreateRandom(666, &random);
    HM_TEST_ASSERT_OK(err);
    for (hm_nint i = 0; i < 1000; i++) {
        hm_int32 random_int = hmRandomGetNextFloat(&random);
        HM_TEST_ASSERT(random_int >= 0.0 && random_int <= 1.0);
    }
    err = hmRandomDispose(&random);
    HM_TEST_ASSERT_OK(err);
}

static void test_can_generate_seed()
{
    hmGenerateSeed();
    /* Nothing to do, just a test the function works. */
}

void test_random()
{
    HM_TEST_SUITE_BEGIN("Random");
        HM_TEST_RUN(test_random_generates_int_sequence);
        HM_TEST_RUN(test_random_generates_float_sequence);
        HM_TEST_RUN(test_can_generate_seed);
    HM_TEST_SUITE_END();
}
