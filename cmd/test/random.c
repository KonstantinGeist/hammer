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
#include <collections/hashmap.h>

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
        hm_float64 random_int = hmRandomGetNextFloat(&random);
        HM_TEST_ASSERT(random_int >= 0.0 && random_int <= 1.0);
    }
    err = hmRandomDispose(&random);
    HM_TEST_ASSERT_OK(err);
}

/* How we actually test seed generation: we generate N seeds and see if there are any duplicates.
   Duplicates are possible but we assume that no more frequent than M.
   There's a still a chance that the tests may fail if there are more than M duplicates by chance alone,
   but it's very unlikely to happen. This test is useful to root out very obvious/serious problems with seed generation. */
static void test_can_generate_seed()
{
    #define SEED_N 10
    #define SEED_M 1
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmHashMap hash_map;
    err = hmCreateHashMap(
        &allocator,
        HM_NULL, /* hash_func */
        HM_NULL, /* equals_func */
        HM_NULL, /* key_dispose_func */
        HM_NULL, /* value_dispose_func */
        sizeof(hm_int32),
        sizeof(hm_int32),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        0,
        &hash_map
    );
    HM_TEST_ASSERT_OK(err);
    for (hm_nint i = 0; i < SEED_N; i++) {
        hm_int32 seed = hmGenerateSeed();
        err = hmHashMapPut(&hash_map, &seed, &seed);
        HM_TEST_ASSERT_OK(err);
    }
    /* If there are duplicates, the hash map will report fewer elements. */
    HM_TEST_ASSERT(hmHashMapGetCount(&hash_map) >= SEED_N - SEED_M);
    err = hmHashMapDispose(&hash_map);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

HM_TEST_SUITE_BEGIN(random)
    HM_TEST_RUN_WITHOUT_OOM(test_random_generates_int_sequence)
    HM_TEST_RUN_WITHOUT_OOM(test_random_generates_float_sequence)
    HM_TEST_RUN_WITHOUT_OOM(test_can_generate_seed)
HM_TEST_SUITE_END()
