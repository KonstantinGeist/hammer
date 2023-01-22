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
#include <threading/process.h>
#include <core/environment.h>
#include <collections/array.h>

#define HM_PROCESS_TEST_ENV_VAR_KEY "HM_PROCESS_TEST"
#define HM_PROCESS_TEST_ENV_VAR_VALUE "true"
#define HM_PROCESS_TEST_EXIT_CODE 113

hm_bool is_process_test(hmAllocator* allocator)
{
    hmString value;
    hmError err = hmGetEnvironmentVariable(allocator, HM_PROCESS_TEST_ENV_VAR_KEY, &value);
    HM_TEST_ASSERT_OK(err);
    hm_bool result = hmStringEqualsToCString(&value, HM_PROCESS_TEST_ENV_VAR_VALUE);
    err = hmStringDispose(&value);
    HM_TEST_ASSERT_OK(err);
    return result;
}

int get_process_test_exit_code()
{
    return HM_PROCESS_TEST_EXIT_CODE;
}

static hmError create_args_array(hmAllocator* allocator, hmArray* in_args)
{
    hmString arg1, arg2;
    HM_TRY(hmCreateStringFromCString(allocator, "arg1", &arg1));
    HM_TRY(hmCreateStringFromCString(allocator, "arg2", &arg2));
    HM_TRY(hmCreateArray(allocator, sizeof(hmString), HM_ARRAY_DEFAULT_CAPACITY, &hmStringDisposeFunc, in_args));
    HM_TRY(hmArrayAdd(in_args, &arg1));
    return hmArrayAdd(in_args, &arg2);
}

static hmError create_env_vars_array(hmAllocator* allocator, hmHashMap* in_vars)
{
    HM_TRY(hmCreateHashMapWithStringKeys(
        allocator,
        &hmStringDisposeFunc,
        sizeof(hmString),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        0,
        in_vars
    ));
    hmString env_var_key, env_var_value;
    /* WARNING Don't remove this, or the tests executable (./hammer-tests) can become a fork bomb!
       (The tests executable launches itself to check processes can be started correctly.) */
    HM_TRY(hmCreateStringFromCString(allocator, HM_PROCESS_TEST_ENV_VAR_KEY, &env_var_key));
    HM_TRY(hmCreateStringFromCString(allocator, HM_PROCESS_TEST_ENV_VAR_VALUE, &env_var_value));
    return hmHashMapPut(in_vars, &env_var_key, &env_var_value);
}

static void test_can_start_process_impl(hm_bool is_success_scenario)
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmString exe_path;
    hmError err;
    if (is_success_scenario) {
        err = hmGetExecutableFilePath(&allocator, &exe_path);
    } else {
        err = hmCreateStringFromCString(&allocator, "non_existing", &exe_path);
    }
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmArray args;
    err = create_args_array(&allocator, &args);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmHashMap environment_vars;
    err = create_env_vars_array(&allocator, &environment_vars);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmStartProcessOptions options;
    options.environment_vars_opt = &environment_vars;
    options.wait_for_exit = HM_TRUE;
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hmProcess process;
    err = hmStartProcess(&allocator, &exe_path, &args, &options, &process);
    if (is_success_scenario) {
        HM_TEST_ASSERT_OK_OR_OOM(err);
        if (err == HM_OK) {
            HM_TEST_ASSERT(hmProcessHasExited(&process));
            HM_TEST_ASSERT(hmProcessGetExitCode(&process) == HM_PROCESS_TEST_EXIT_CODE);
        }
    } else {
        HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND || err == HM_ERROR_OUT_OF_MEMORY);
        HM_TEST_ASSERT(!hmProcessHasExited(&process));
    }
    err = hmProcessDispose(&process);
    HM_TEST_ASSERT(err == HM_OK || err == HM_ERROR_OUT_OF_MEMORY);
HM_TEST_ON_FINALIZE
    err = hmHashMapDispose(options.environment_vars_opt);
    HM_TEST_ASSERT_OK(err);
    err = hmArrayDispose(&args);
    HM_TEST_ASSERT_OK(err);
    err = hmStringDispose(&exe_path);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_can_start_process()
{
    test_can_start_process_impl(HM_TRUE);
}

static void test_cannot_start_process_which_cannot_be_found()
{
    test_can_start_process_impl(HM_FALSE);
}

HM_TEST_SUITE_BEGIN(processes)
    HM_TEST_RUN(test_can_start_process)
    HM_TEST_RUN(test_cannot_start_process_which_cannot_be_found)
HM_TEST_SUITE_END()
