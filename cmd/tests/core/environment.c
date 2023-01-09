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
#include <core/environment.h>
#include <threading/thread.h>

#include <string.h> /* for strlen(..) */

#define LAST_EXECUTABLE_FILE_PATH_PART "/hammer-tests"

static void test_tick_count_grows_monotonically()
{
    hm_millis first_tick_count = hmGetTickCount();
    hmError err = hmSleep(100);
    HM_TEST_ASSERT_OK(err);
    hm_millis second_tick_count = hmGetTickCount();
    HM_TEST_ASSERT(second_tick_count > first_tick_count);
}

static void test_can_get_processor_count()
{
    hm_nint processor_count = hmGetProcessorCount();
    HM_TEST_ASSERT(processor_count > 0);
}

static void test_can_get_executable_file_path()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    hmString executable_file_path;
    hmError err = hmGetExecutableFilePath(&allocator, &executable_file_path);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(hmStringGetLength(&executable_file_path) > 0);
    hm_nint last_part_length = strlen(LAST_EXECUTABLE_FILE_PATH_PART);
    const char* c_ctring = hmStringGetCString(&executable_file_path) + hmStringGetLength(&executable_file_path) - last_part_length;
    for(hm_nint i = 0; i < last_part_length; i++) {
        HM_TEST_ASSERT(LAST_EXECUTABLE_FILE_PATH_PART[i] == c_ctring[i]);
    }
#ifdef HM_UNIX
    HM_TEST_ASSERT(hmStringGetCString(&executable_file_path)[0] == '/');
#endif
HM_TEST_ON_FINALIZE
    if (err == HM_OK) {
        err = hmStringDispose(&executable_file_path);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

HM_TEST_SUITE_BEGIN(environment)
    HM_TEST_RUN_WITHOUT_OOM(test_tick_count_grows_monotonically)
    HM_TEST_RUN_WITHOUT_OOM(test_can_get_processor_count)
    HM_TEST_RUN(test_can_get_executable_file_path);
HM_TEST_SUITE_END()
