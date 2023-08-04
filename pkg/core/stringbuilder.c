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

#include <core/stringbuilder.h>
#include <core/allocator.h>
#include <core/math.h>
#include <core/string.h>
#include <core/utils.h>

#include <stdarg.h> /* for va_list, va_start(..), va_end(..) and va_arg(..) */
#include <string.h> /* for strlen(..) */

hmError hmCreateStringBuilder(hmAllocator* allocator, hmStringBuilder* in_string_builder)
{
    HM_TRY(hmCreateArray(allocator, sizeof(char), HM_ARRAY_DEFAULT_CAPACITY, HM_NULL, &in_string_builder->buffer));
    in_string_builder->allocator = allocator;
    return HM_OK;
}

hmError hmStringBuilderDispose(hmStringBuilder* string_builder)
{
    return hmArrayDispose(&string_builder->buffer);
}

hmError hmStringBuilderAppendCString(hmStringBuilder* string_builder, const char* c_string)
{
    hm_nint length = strlen(c_string);
    return hmArrayAddRange(&string_builder->buffer, (void*)c_string, length);
}

hmError hmStringBuilderAppendCStrings(hmStringBuilder* string_builder, ...)
{
    hmError err = HM_OK;
    va_list vl;
    va_start(vl, string_builder);
    const char* c_string;
    while ((c_string = va_arg(vl, const char*))) {
        err = hmStringBuilderAppendCString(string_builder, c_string);
        if (err != HM_OK) {
            break;
        }
    }
    va_end(vl);
    return err;
}

hmError hmStringBuilderAppendCStringWithLength(hmStringBuilder* string_builder, const char* c_string, hm_nint length)
{
    return hmArrayAddRange(&string_builder->buffer, (void*)c_string, length);
}

hmError hmStringBuilderToString(hmStringBuilder* string_builder, hmAllocator* allocator_opt, hmString* in_string)
{
    if (!allocator_opt) {
        allocator_opt = string_builder->allocator;
    }
    const char* buffer_chars = hmArrayGetRaw(&string_builder->buffer, const char);
    hm_nint buffer_length_in_bytes = hmArrayGetCount(&string_builder->buffer);
    return hmCreateStringFromCStringWithLengthInBytes(allocator_opt, buffer_chars, buffer_length_in_bytes, in_string);
}

hmError hmStringBuilderToStringWithStartIndexAndLengthInBytes(
   hmStringBuilder* string_builder,
   hmAllocator*     allocator_opt,
   hm_nint          start_index,
   hm_nint          length_in_bytes,
   hmString*        in_string
)
{
    if (!allocator_opt) {
        allocator_opt = string_builder->allocator;
    }
    const char* buffer_chars = hmArrayGetRaw(&string_builder->buffer, const char);
    hm_nint buffer_length_in_bytes = hmArrayGetCount(&string_builder->buffer);
    if (start_index > buffer_length_in_bytes) {
        return HM_ERROR_OUT_OF_RANGE;
    }
    hm_nint end_index = 0;
    HM_TRY(hmAddNint(start_index, length_in_bytes, &end_index));
    if (end_index > buffer_length_in_bytes) {
        return HM_ERROR_OUT_OF_RANGE;
    }
    hm_nint chars_with_start_index_address = 0;
    HM_TRY(hmAddNint(hmCastPointerToNint(buffer_chars), start_index, &chars_with_start_index_address));
    return hmCreateStringFromCStringWithLengthInBytes(
        allocator_opt,
        hmCastNintToPointer(chars_with_start_index_address, char*),
        length_in_bytes,
        in_string
    );
}

hmError hmStringBuilderToCString(hmStringBuilder* string_builder, hmAllocator* allocator_opt, char** out_c_string)
{
    if (!allocator_opt) {
        allocator_opt = string_builder->allocator;
    }
    const char* buffer_chars = hmArrayGetRaw(&string_builder->buffer, const char);
    hm_nint length_in_bytes = hmArrayGetCount(&string_builder->buffer);
    hm_nint length_in_bytes_with_null = 0;
    HM_TRY(hmAddNint(length_in_bytes, 1, &length_in_bytes_with_null));
    char* result = (char*)hmAlloc(allocator_opt, length_in_bytes_with_null);
    if (!result) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmCopyMemory(result, buffer_chars, length_in_bytes);
    result[length_in_bytes] = '\0'; /* null terminator */
    *out_c_string = result;
    return HM_OK;
}

hmError hmStringBuilderClear(hmStringBuilder* string_builder)
{
    return hmArrayClear(&string_builder->buffer);
}
