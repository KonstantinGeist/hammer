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

#ifndef HM_STRINGBUILDER_H
#define HM_STRINGBUILDER_H

#include <core/common.h>
#include <core/string.h>
#include <collections/array.h>

struct _hmAllocator;

typedef struct {
    struct _hmAllocator* allocator;
    hmArray              array;
} hmStringBuilder;

/* Creates a string builder, which allows to efficiently construct strings. */
hmError hmCreateStringBuilder(struct _hmAllocator* allocator, hmStringBuilder* in_string_builder);
hmError hmStringBuilderDispose(hmStringBuilder* string_builder);
/* Appends a C string to the end of the string being constructed. */
hmError hmStringBuilderAppendCString(hmStringBuilder* string_builder, const char* c_string);
/* Creates a string from the string builder.
   `allocator` is the allocator to create the string with. If it's not provided, the string builder's allocator
   will be reused. */
hmError hmStringBuilderToString(hmStringBuilder* string_builder, struct _hmAllocator* allocator, hmString* in_string);

#endif /* HM_STRINGBUILDER_H */
