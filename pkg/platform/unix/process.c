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

#include <threading/process.h>
#include <core/allocator.h>
#include <core/math.h>
#include <core/stringbuilder.h>

#define HM_UNIX_ARGS_BUFFER_SIZE 512

static hmError hmConvertHammerProcessArgsToUnix(struct _hmAllocator* allocator, hmString* path, hmArray* hammer_args, char*** out_unix_args);
static void hmDisposeUnixProcessArgs(struct _hmAllocator* allocator, char** unix_args);
static hmError hmConvertHammerEnvironmentVarsToUnix(struct _hmAllocator* allocator, hmHashMap* hammer_env_vars, char*** out_unix_env_vars);
static void hmDisposeUnixEnvironmentArgs(struct _hmAllocator* allocator, char** unix_env_vars, hm_nint count);

/* TODO remove */
#include <stdio.h>

static void debugPrint(char** unix_args)
{
    if (!unix_args) {
        return;
    }
    char* unix_arg;
    while ((unix_arg = *unix_args)) {
        printf("%s\n", unix_arg);
        unix_args++;
    }
    printf("\n");
}

hmError hmStartProcess(
    struct _hmAllocator* allocator,
    hmString* path,
    hmArray* args,
    hmStartProcessOptions* options,
    hmProcess* in_process
)
{
    char unix_args_buffer[HM_UNIX_ARGS_BUFFER_SIZE];
    hmAllocator buffer_allocator; /* note: not required to dispose */
    HM_TRY(hmCreateBufferAllocator(unix_args_buffer, sizeof(unix_args_buffer), allocator, &buffer_allocator));
    char** unix_args = HM_NULL;
    char** unix_env_vars = HM_NULL;
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmConvertHammerProcessArgsToUnix(&buffer_allocator, path, args, &unix_args));
    if (options && options->environment_vars) {
        HM_TRY_OR_FINALIZE(err, hmConvertHammerEnvironmentVarsToUnix(&buffer_allocator, options->environment_vars, &unix_env_vars));
    }
    /* TODO start process here */
    debugPrint(unix_args);
    debugPrint(unix_env_vars);
HM_ON_FINALIZE
    /* cppcheck-suppress shadowFunction */
    /* cppcheck-suppress unreadVariable */
    hmDisposeUnixProcessArgs(&buffer_allocator, unix_args);
    /* cppcheck-suppress shadowFunction */
    /* cppcheck-suppress unreadVariable */
    hmDisposeUnixEnvironmentArgs(&buffer_allocator, unix_env_vars, HM_NINT_MAX);
    return err;
}

hmError hmProcessDispose(hmProcess* process)
{
    return HM_OK;
}

/* Converts the arguments to the format specified by the `execv` family of functions (list of C strings terminated with a NULL).
   Note that the function assumes the unix arg array won't outlive the original array since it directly references the char buffers inside it. */
static hmError hmConvertHammerProcessArgsToUnix(struct _hmAllocator* allocator, hmString* path, hmArray* hammer_args, char*** out_unix_args)
{
    hm_nint arg_count = hmArrayGetCount(hammer_args);
    hm_nint alloc_size = 0;
    /* Accounts for the null terminator and the convention that the first argument points to the filename. */
    HM_TRY(hmAddMulNint(sizeof(char*) * 2, arg_count, sizeof(char*), &alloc_size));
    char** unix_args = (char**)hmAlloc(allocator, alloc_size);
    if (!unix_args) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmString* raw_hammer_strings = hmArrayGetRaw(hammer_args, hmString);
    /* "The first argument, by convention, should point to the filename associated with the file being executed". */
    unix_args[0] = (char*)hmStringGetCString(path);
    for (hm_nint i = 0; i < arg_count; i++) {
        hmString* hammer_arg = &raw_hammer_strings[i];
        unix_args[i + 1] = (char*)hmStringGetCString(hammer_arg); /* no overflow-safe math for "i + 1" because prevalidated in alloc_size */
    }
    unix_args[arg_count + 1] = HM_NULL;
    *out_unix_args = unix_args;
    return HM_OK;
}

static void hmDisposeUnixProcessArgs(struct _hmAllocator* allocator, char** unix_args)
{
    if (!unix_args) {
        return;
    }
    hmFree(allocator, unix_args);
}

typedef struct {
    hmStringBuilder* string_builder;
    char**           unix_env_vars;
    hm_nint          unix_env_vars_index;
} hmEnvironmentMapEnumerateFuncContext;

static hmError hmEnvironmentMapEnumerateFunc(void* key, void* value, void* user_data)
{
    const char* key_c_string = hmStringGetCString((hmString*)key);
    const char* value_c_string = hmStringGetCString((hmString*)value);
    hmEnvironmentMapEnumerateFuncContext* context = (hmEnvironmentMapEnumerateFuncContext*)user_data;
    HM_TRY(hmStringBuilderClear(context->string_builder));
    HM_TRY(hmStringBuilderAppendCStrings(context->string_builder, key_c_string, "=", value_c_string, HM_NULL));
    char* c_string = HM_NULL;
    HM_TRY(hmStringBuilderToCString(context->string_builder, HM_NULL, &c_string));
    context->unix_env_vars[context->unix_env_vars_index] = c_string;
    context->unix_env_vars_index++;
    return HM_OK;
}

/* Similar to hmConvertHammerProcessArgsToUnix(..) (see). */
static hmError hmConvertHammerEnvironmentVarsToUnix(struct _hmAllocator* allocator, hmHashMap* hammer_env_vars, char*** out_unix_env_vars)
{
    hm_nint env_var_count = hmHashMapGetCount(hammer_env_vars);
    hm_nint alloc_size = 0;
    /* Accounts for the null terminator. */
    HM_TRY(hmAddMulNint(sizeof(char*), env_var_count, sizeof(char*), &alloc_size));
    char** unix_env_vars = (char**)hmAlloc(allocator, alloc_size);
    if (!unix_env_vars) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    hmStringBuilder string_builder;
    hm_bool is_string_builder_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmCreateStringBuilder(allocator, &string_builder));
    is_string_builder_initialized = HM_TRUE;
    hmEnvironmentMapEnumerateFuncContext context;
    context.string_builder = &string_builder;
    context.unix_env_vars = unix_env_vars;
    context.unix_env_vars_index = 0;
    HM_TRY_OR_FINALIZE(err, hmHashMapEnumerate(hammer_env_vars, &hmEnvironmentMapEnumerateFunc, &context));
    unix_env_vars[env_var_count] = HM_NULL;
HM_ON_FINALIZE
    if (is_string_builder_initialized) {
        err = hmMergeErrors(err, hmStringBuilderDispose(&string_builder));
    }
    if (err != HM_OK) {
        hmDisposeUnixEnvironmentArgs(allocator, unix_env_vars, context.unix_env_vars_index);
    } else {
        *out_unix_env_vars = unix_env_vars;
    }
    return err;
}

static void hmDisposeUnixEnvironmentArgs(struct _hmAllocator* allocator, char** unix_env_vars, hm_nint count)
{
    if (!unix_env_vars) {
        return;
    }
    for (hm_nint i = 0; i < count && unix_env_vars[i]; i++) {
        hmFree(allocator, unix_env_vars[i]);
    }
    hmFree(allocator, unix_env_vars);
}
