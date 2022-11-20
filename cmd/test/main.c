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

#include "tests.h"
#include "common.h"

int main()
{
    HM_TEST_LOG("*****************");
    HM_TEST_LOG("Starting tests...");
    HM_TEST_LOG("*****************");
    test_allocators();
    test_readers();
    test_arrays();
    test_modules();
    test_strings();
    test_utils();
    test_hashmaps();
    test_hashes();
    test_errors();
    test_queues();
    test_mutexes();
    test_wait_objects();
    test_threads();
    test_environment();
    HM_TEST_LOG("***************");
    HM_TEST_LOG("Tests finished.");
    HM_TEST_LOG("***************");
    return 0;
}
