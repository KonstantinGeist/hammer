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
#include <collections/array.h>

static hmError create_exe_path(hmAllocator* allocator, hmString* in_path)
{
    return hmCreateStringFromCString(allocator, "exe_path", in_path);
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
    HM_TRY(hmCreateStringFromCString(allocator, "KEY", &env_var_key));
    HM_TRY(hmCreateStringFromCString(allocator, "VALUE", &env_var_value));
    return hmHashMapPut(in_vars, &env_var_key, &env_var_value);
}

static void test_can_start_process()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmString exe_path;
    hmError err = create_exe_path(&allocator, &exe_path);
    hmArray args;
    err = create_args_array(&allocator, &args);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmHashMap environment_vars;
    err = create_env_vars_array(&allocator, &environment_vars);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmStartProcessOptions options;
    options.environment_vars = &environment_vars;
    options.wait_for_exit = HM_TRUE;
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hmProcess process;
    err = hmStartProcess(&allocator, &exe_path, &args, &options, &process);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmProcessDispose(&process);
    HM_TEST_ASSERT(err == HM_OK || err == HM_ERROR_OUT_OF_MEMORY);
HM_TEST_ON_FINALIZE
    err = hmHashMapDispose(options.environment_vars);
    HM_TEST_ASSERT_OK(err);
    err = hmArrayDispose(&args);
    HM_TEST_ASSERT_OK(err);
    err = hmStringDispose(&exe_path);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

HM_TEST_SUITE_BEGIN(processes)
    HM_TEST_RUN(test_can_start_process)
HM_TEST_SUITE_END()
