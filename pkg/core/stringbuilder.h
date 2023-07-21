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
#include <core/allocator.h>
#include <core/string.h>
#include <collections/array.h>

typedef struct {
    hmAllocator* allocator;
    hmArray      array;     /* A string builder is basically an array with additional logic on top. */
} hmStringBuilder;

/* Creates a string builder, which allows to efficiently construct strings. */
hmError hmCreateStringBuilder(hmAllocator* allocator, hmStringBuilder* in_string_builder);
hmError hmStringBuilderDispose(hmStringBuilder* string_builder);
/* Appends a C string to the end of the string being constructed. */
hmError hmStringBuilderAppendCString(hmStringBuilder* string_builder, const char* c_string);
/* Same as hmStringBuilderAppendCString(..), except allows to append several C strings of type "const char*" at once.
   Expects the last string to be HM_NULL for termination. The array can be partially appended after an out-of-memory
   error happens. */
hmError hmStringBuilderAppendCStrings(hmStringBuilder* string_builder, ...);
/* Same as hmStringBuilderAppendCString(..), except uses the provided argument for length instead of null termination. */
hmError hmStringBuilderAppendCStringWithLength(hmStringBuilder* string_builder, const char* c_string, hm_nint length);
/* Creates a string from the string builder.
  `allocator` is the allocator to create the string with. If it's not provided, the string builder's allocator
   will be reused. */
hmError hmStringBuilderToString(hmStringBuilder* string_builder, hmAllocator* allocator_opt, hmString* in_string);
/* Same as hmStringBuilderToString, except creates a C string. */
hmError hmStringBuilderToCString(hmStringBuilder* string_builder, hmAllocator* allocator_opt, char** out_c_string);
/* Clears the string builder, allowing the instance to be reused in a different case: the length is reset to 0
   and all the previous content is wiped out. */
hmError hmStringBuilderClear(hmStringBuilder* string_builder);
/* Returns the length of the string builder (the number of appended characters). */
#define hmStringBuilderGetLength(sb) (hmArrayGetCount(&((sb)->array)))

#endif /* HM_STRINGBUILDER_H */
