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

HM_TEST_DECLARE_SUITE(allocators)
HM_TEST_DECLARE_SUITE(readers)
HM_TEST_DECLARE_SUITE(line_readers)
HM_TEST_DECLARE_SUITE(arrays)
HM_TEST_DECLARE_SUITE(strings)
HM_TEST_DECLARE_SUITE(string_pools)
HM_TEST_DECLARE_SUITE(string_builders)
HM_TEST_DECLARE_SUITE(utils)
HM_TEST_DECLARE_SUITE(hash_maps)
HM_TEST_DECLARE_SUITE(hashes)
HM_TEST_DECLARE_SUITE(errors)
HM_TEST_DECLARE_SUITE(queues)
HM_TEST_DECLARE_SUITE(signatures)
HM_TEST_DECLARE_SUITE(modules)
HM_TEST_DECLARE_SUITE(http_requests)
HM_TEST_DECLARE_SUITE(sockets)
HM_TEST_DECLARE_SUITE(mutexes)
HM_TEST_DECLARE_SUITE(waitable_events)
HM_TEST_DECLARE_SUITE(threads)
HM_TEST_DECLARE_SUITE(processes)
HM_TEST_DECLARE_SUITE(environment)
HM_TEST_DECLARE_SUITE(random)
HM_TEST_DECLARE_SUITE(math)
HM_TEST_DECLARE_SUITE(workers)

hm_bool is_process_test(hmAllocator* allocator);
int get_process_test_exit_code();
