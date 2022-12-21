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
#include <core/math.h>
#include <core/stringbuilder.h>
#include <platform/unix/common.h>

#include <inttypes.h> /* for PRId32 */
#include <stdio.h>    /* for fopen(..) and sprintf(..) */
#include <unistd.h>   /* for sysconf(..), _SC_NPROCESSORS_ONLN and getpid(..) */

#define HM_COMMAND_LINE_BUFFER_SIZE 1024

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

hmError hmGetCommandLineArguments(struct _hmAllocator* allocator, hmArray* in_array)
{
    HM_TRY(hmCreateArray(allocator, sizeof(hmString), HM_DEFAULT_ARRAY_CAPACITY, &hmStringDisposeFunc, in_array));
    hmError err = HM_OK;
    hmStringBuilder string_builder;
    hm_bool is_string_builder_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmCreateStringBuilder(allocator, &string_builder));
    is_string_builder_initialized = HM_TRUE;
    char file_name[64];
    pid_t proc_id = getpid();
    /* Not quite portable to cast pid_t to int32, but in GCC it's an integer and it's very unlikely
       to be larger than int32. However, if this invariant is not upheld, we may end up reading arguments
       of a different, unrelated process if there's a wraparound. */
    sprintf(file_name, "/proc/%" PRId32 "/cmdline", (hm_int32)proc_id);
    FILE* file = fopen(file_name, "rb");
    if (!file) {
        err = hmMergeErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
        HM_FINALIZE;
    }
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
    if (is_string_builder_initialized) {
        err = hmMergeErrors(err, hmStringBuilderDispose(&string_builder));
    }
    if (file && fclose(file)) {
        err = hmMergeErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
    }
    if (err != HM_OK) {
        err = hmMergeErrors(err, hmArrayDispose(in_array));
    }
    return err;
}
