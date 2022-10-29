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
#include "../src/hash.h"

static void test_can_hash_empty_array()
{
    char bytes[1];
    hm_uint32 hash = hmHash(&bytes[0], 0);
    HM_TEST_ASSERT(hash == 0);
}

static void test_can_hash_non_empty_array()
{
    char bytes[8] = {'0', '1', '2', '3', '4', '5', '6', '7'};
    hm_uint32 hash = hmHash(&bytes[0], sizeof(char)*8);
    HM_TEST_ASSERT(hash == 1335117771); // precomputed
}

void test_hashes()
{
    test_can_hash_empty_array();
    test_can_hash_non_empty_array();
}
