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

#include <core/environment.h>
#include <core/allocator.h>
#include <core/math.h>
#include <core/stringbuilder.h>
#include <platform/unix/common.h>

#include <inttypes.h> /* for PRId32 */
#include <stdio.h>    /* for fopen(..), fread(..), fclose(..) and sprintf(..) */
#include <stdlib.h>   /* for getenv(..) */
#include <unistd.h>   /* for sysconf(..), _SC_NPROCESSORS_ONLN and getpid(..) */

#define HM_COMMAND_LINE_BUFFER_SIZE 1024
#define HM_EXECUTABLE_FILE_PATH_BUFFER_SIZE 1024
#define HM_SYSTEM_FILE_NAME_BUFFER_SIZE 64

static hmError hmFormatWithCurrentProcessId(
    struct _hmAllocator* allocator,
    char buffer[HM_SYSTEM_FILE_NAME_BUFFER_SIZE],
    const char* before_part,
    const char* after_part
);

hm_millis hmGetTickCount()
{
    struct timespec ts = hmGetCurrentTimeSpec(HM_TRUE);
    return hmConvertTimeSpecToMilliseconds(&ts);
}

hm_nint hmGetProcessorCount()
{
#ifdef _SC_NPROCESSORS_ONLN
    /* Technically non-standard, but we'll simply return 1 processor if no such
       name is found. */
    long result = sysconf(_SC_NPROCESSORS_ONLN);
    /* Assume 1 processor if the call to sysconf wasn't successful. */
    return result < 1 ? 1 : (hm_nint)result;
#else
    return 1;
#endif /* _SC_NPROCESSORS_ONLN */
}

hmError hmGetEnvironmentVariable(struct _hmAllocator* allocator, const char* name, hmString* in_value)
{
    char* value = getenv(name);
    if (!value) {
        return hmCreateEmptyStringView(in_value);
    }
    return hmCreateStringFromCString(allocator, value, in_value);
}

hmError hmGetCommandLineArguments(struct _hmAllocator* allocator, hmArray* in_array)
{
    char system_file_name[HM_SYSTEM_FILE_NAME_BUFFER_SIZE];
    HM_TRY(hmFormatWithCurrentProcessId(allocator, system_file_name, "/proc/", "/cmdline"));
    FILE* file = fopen(system_file_name, "rb");
    if (!file) {
        return HM_ERROR_PLATFORM_DEPENDENT;
    }
    hmError err = HM_OK;
    hmStringBuilder string_builder;
    hm_bool is_array_initialized = HM_FALSE,
            is_string_builder_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmCreateArray(allocator, sizeof(hmString), HM_DEFAULT_ARRAY_CAPACITY, &hmStringDisposeFunc, in_array));
    is_array_initialized = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmCreateStringBuilder(allocator, &string_builder));
    is_string_builder_initialized = HM_TRUE;
    char buffer[HM_COMMAND_LINE_BUFFER_SIZE];
    hm_nint read_bytes = 0;
    hm_nint arg_count = 0; /* to skip the first element, which is the executable name we don't need in our API */
    while ((read_bytes = fread(buffer, 1, HM_COMMAND_LINE_BUFFER_SIZE, file)) != 0) {
        hm_nint last_i = 0;
        for (hm_nint i = 0; i < read_bytes; i++) {
            char c = buffer[i];
            if (!c) { /* null terminator found */
                if (arg_count > 0) {
                    HM_TRY_OR_FINALIZE(err, hmStringBuilderAppendCStringWithLength(&string_builder, buffer + last_i, i - last_i));
                    hmString string;
                    HM_TRY_OR_FINALIZE(err, hmStringBuilderToString(&string_builder, HM_NULL, &string));
                    err = hmArrayAdd(in_array, &string);
                    if (err != HM_OK) {
                        err = hmMergeErrors(err, hmStringDispose(&string));
                        HM_FINALIZE;
                    }
                }
                HM_TRY_OR_FINALIZE(err, hmStringBuilderClear(&string_builder));
                last_i = i + 1; /* no overflow-safe math because `i` is guaranteed to be less than HM_COMMAND_LINE_BUFFER_SIZE */
                HM_TRY(hmAddNint(arg_count, 1, &arg_count));
            }
        }
        if (last_i != read_bytes) {
            if (arg_count > 0) {
                HM_TRY_OR_FINALIZE(err, hmStringBuilderAppendCStringWithLength(&string_builder, buffer + last_i, read_bytes - last_i));
            }
        }
    }
HM_ON_FINALIZE
    if (fclose(file)) {
        err = hmMergeErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
    }
    if (is_string_builder_initialized) {
        err = hmMergeErrors(err, hmStringBuilderDispose(&string_builder));
    }
    if (err != HM_OK && is_array_initialized) {
        err = hmMergeErrors(err, hmArrayDispose(in_array));
    }
    return err;
}

hmError hmGetExecutableFilePath(struct _hmAllocator* allocator, hmString* in_file_path)
{
    char system_file_name[HM_SYSTEM_FILE_NAME_BUFFER_SIZE];
    HM_TRY(hmFormatWithCurrentProcessId(allocator, system_file_name, "/proc/", "/exe"));
    hmError err = HM_OK;
    hm_nint buffer_size = HM_EXECUTABLE_FILE_PATH_BUFFER_SIZE;
    char* buffer = HM_NULL;
    for (;;) {
        buffer = hmAlloc(allocator, buffer_size);
        if (!buffer) {
            return HM_ERROR_OUT_OF_MEMORY;
        }
        int ret = readlink(system_file_name, buffer, buffer_size);
        if (ret <= 0) {
            err = hmMergeErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
            HM_FINALIZE;
        }
        if (ret < buffer_size - 1) { /* -1 gives some space for the null terminator. No overflow-safe math because buffer_size > 0 */
            buffer[ret] = 0; /* Ensures proper null termination. */
            break;
        }
        hmFree(allocator, buffer);
        buffer = HM_NULL;
        HM_TRY_OR_FINALIZE(err, hmMulNint(buffer_size, 2, &buffer_size));
    }
HM_ON_FINALIZE
    if (err == HM_OK) {
        err = hmCreateStringFromCString(allocator, buffer, in_file_path);
    }
    hmFree(allocator, buffer);
    return err;
}

static hmError hmFormatWithCurrentProcessId(
    struct _hmAllocator* allocator,
    char buffer[HM_SYSTEM_FILE_NAME_BUFFER_SIZE],
    const char* before_part,
    const char* after_part
)
{
    hmStringBuilder string_builder;
    HM_TRY(hmCreateStringBuilder(allocator, &string_builder));
    hmError err = HM_OK;
    hmString format;
    hm_bool is_format_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmStringBuilderAppendCStrings(&string_builder, before_part, "%", PRId32, after_part, HM_NULL));
    HM_TRY_OR_FINALIZE(err, hmStringBuilderToString(&string_builder, allocator, &format));
    is_format_initialized = HM_TRUE;
    pid_t process_id = getpid();
    /* Not quite portable to cast pid_t to int32, but in GCC it's an integer and it's very unlikely
       to be larger than int32. However, if this invariant is not upheld, we may end up reading arguments
       of a different, unrelated process if there's a wraparound. Also, see hmGetCommandLineArguments(..) */
    sprintf(buffer, hmStringGetCString(&format), (hm_int32)process_id);
HM_ON_FINALIZE
    if (is_format_initialized) {
        err = hmMergeErrors(err, hmStringDispose(&format));
    }
    return hmMergeErrors(err, hmStringBuilderDispose(&string_builder));
}
