// *****************************************************************************
//
//  Copyright (c) Konstantin Geist. All rights reserved.
//
//  The use and distribution terms for this software are contained in the file
//  named License.txt, which can be found in the root of this distribution.
//  By using this software in any fashion, you are agreeing to be bound by the
//  terms of this license.
//
//  You must not remove this notice, or any other, from this software.
//
// *****************************************************************************

#include "string.h"
#include "allocator.h"

hmError hmCreateStringFromCString(struct _hmAllocator* allocator, const char* content, hmString* in_string)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hm_nint length = strlen(content);
    char* content_copy = hmAlloc(allocator, length+1); // including null terminator
    if (!content_copy) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    memcpy(content_copy, content, length+1); // including null terminator
    in_string->content = content_copy;
    in_string->allocator = allocator;
    in_string->length = length;
    return HM_OK;
}

hmError hmStringDispose(hmString* string)
{
    hmFree(string->allocator, string->content);
    return HM_OK;
}

hm_bool hmStringEqualsToCString(hmString* string, const char* content)
{
    if (!content) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    return strcmp(string->content, content) == 0;
}
