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

#include "tests.h"
#include "common.h"

#include <core/allocator.h>
#include <core/environment.h>
#include <core/string.h>
#include <core/utils.h>
#include <collections/array.h>

static void run_tests(hmTestSelector* test_selector)
{
    HM_TEST_LOG("*****************");
    HM_TEST_LOG("Starting tests...");
    HM_TEST_LOG("*****************");
    {
        HM_TEST_RUN_SUITE(allocators);
        HM_TEST_RUN_SUITE(readers);
        HM_TEST_RUN_SUITE(arrays);
        HM_TEST_RUN_SUITE(strings);
        HM_TEST_RUN_SUITE(string_pools);
        HM_TEST_RUN_SUITE(string_builders);
        HM_TEST_RUN_SUITE(utils);
        HM_TEST_RUN_SUITE(hash_maps);
        HM_TEST_RUN_SUITE(hashes);
        HM_TEST_RUN_SUITE(errors);
        HM_TEST_RUN_SUITE(queues);
        HM_TEST_RUN_SUITE(environment);
        HM_TEST_RUN_SUITE(random);
        HM_TEST_RUN_SUITE(math);
        HM_TEST_RUN_SUITE(signatures);
        HM_TEST_RUN_SUITE(modules);
        /* Tests which rely on timing should come last for the faster tests to fail earlier. */
        HM_TEST_RUN_SUITE(mutexes);
        HM_TEST_RUN_SUITE(waitable_events);
        HM_TEST_RUN_SUITE(threads);
        HM_TEST_RUN_SUITE(processes);
        HM_TEST_RUN_SUITE(workers);
    }
    HM_TEST_LOG("***************");
    HM_TEST_LOG("Tests finished.");
    HM_TEST_LOG("***************");
}

int main()
{
    hmAllocator allocator;
    hmArray args;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmGetCommandLineArguments(&allocator, &args);
    HM_TEST_ASSERT_OK(err);
    hmTestSelector selector;
    if (hmArrayGetCount(&args) == 1) {
        hmString suite_name;
        err = hmArrayGet(&args, 0, (void*)&suite_name);
        HM_TEST_ASSERT_OK(err);
        selector.test_suite_name = hmStringGetCString(&suite_name);
    } else {
        selector.test_suite_name = HM_NULL;
    }
    run_tests(&selector);
    err = hmArrayDispose(&args);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    return 0;
}
