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

hmError hmCreateStringBuilder(struct _hmAllocator* allocator, hmStringBuilder* in_string_builder)
{
    HM_TRY(hmCreateArray(allocator, sizeof(char), HM_ARRAY_DEFAULT_CAPACITY, HM_NULL, &in_string_builder->array));
    in_string_builder->allocator = allocator;
    return HM_OK;
}

hmError hmStringBuilderDispose(hmStringBuilder* string_builder)
{
    return hmArrayDispose(&string_builder->array);
}

hmError hmStringBuilderAppendCString(hmStringBuilder* string_builder, const char* c_string)
{
    hm_nint length = strlen(c_string);
    return hmArrayAddRange(&string_builder->array, (void*)c_string, length);
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
    return hmArrayAddRange(&string_builder->array, (void*)c_string, length);
}

hmError hmStringBuilderToString(hmStringBuilder* string_builder, struct _hmAllocator* allocator, hmString* in_string)
{
    if (!allocator) {
        allocator = string_builder->allocator;
    }
    const char* chars = hmArrayGetRaw(&string_builder->array, const char);
    hm_nint count = hmArrayGetCount(&string_builder->array);
    return hmCreateStringFromCStringWithLength(allocator, chars, count, in_string);
}

hmError hmStringBuilderToCString(hmStringBuilder* string_builder, struct _hmAllocator* allocator, char** out_c_string)
{
    if (!allocator) {
        allocator = string_builder->allocator;
    }
    const char* content = hmArrayGetRaw(&string_builder->array, const char);
    hm_nint length = hmArrayGetCount(&string_builder->array);
    hm_nint length_with_null = 0;
    HM_TRY(hmAddNint(length, 1, &length_with_null));
    char* content_copy = (char*)hmAlloc(allocator, length_with_null);
    if (!content_copy) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmCopyMemory(content_copy, content, length);
    content_copy[length] = '\0'; /* null terminator */
    *out_c_string = content_copy;
    return HM_OK;
}

hmError hmStringBuilderClear(hmStringBuilder* string_builder)
{
    return hmArrayClear(&string_builder->array);
}
