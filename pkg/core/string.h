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

#ifndef HM_STRING_H
#define HM_STRING_H

#include <core/common.h>
#include <core/allocator.h>

typedef struct {
    hm_char*     content;
    hmAllocator* allocator;
    hm_nint      length;    /* String's length is remembered to avoid O(n) lookups every time we need a string's length. */
} hmString;

/* Creates a Hammer string from a null-terminated C string. Duplicates the given string and owns it: deallocates the
   internal buffer when the object is disposed of. See also hmCreateStringViewFromCString.
   Strings are immutable. */
hmError hmCreateStringFromCString(hmAllocator* allocator, const char* content, hmString* in_string);
/* Same as hmCreateStringFromCString(..) except it doesn't rely on null termination -- instead, the length is provided
   as an argument (the null terminator is not included in the length).
   It's the responsibility of the caller to make sure there's no buffer overflow if length parameter is larger than
   the actual string. Empty strings with zero length are allowed.
   Strings are immutable. */
hmError hmCreateStringFromCStringWithLength(hmAllocator* allocator, const char* content, hm_nint length, hmString* in_string);
/* Creates a Hammer string from a null-terminated C string. Unlike hmCreateStringFromCString (see), does not duplicate
   the string and does not own the internal buffer. The string view will be invalidated after the referenced string
   is deleted; it's undefined behavior to try to use such a string afterwards. Mostly useful for creating short-lived
   views for reading, for example, as a key to a container (if the container promises to never retain the value).
   String views should not be disposed, but it should be safe to try to dispose them.
   Strings are immutable. */
hmError hmCreateStringViewFromCString(const char* content, hmString* in_string);
/* Creates an empty string view. Same as hmCreateStringViewFromCString("", ..)
   Strings are immutable. */
hmError hmCreateEmptyStringView(hmString* in_string);
hmError hmStringDuplicate(hmAllocator* allocator, hmString* string, hmString* in_duplicate);
hmError hmStringDispose(hmString* string);
hm_bool hmStringEqualsToCString(hmString* string, const char* content);
hm_bool hmStringEquals(hmString* string1, hmString* string2);
/* Hashes a string. For `salt`, see hmHash(..) */
hm_uint32 hmStringHash(hmString* string, hm_uint32 salt);
/* Returns the length of the string. The length may be computed lazily and is cached inside the string. */
hm_nint hmStringGetLength(hmString* string);
/* Returns the raw contents of the string as a null-terminated C string. The contents should stay immutable
   because certain values, such as the string's length, can be cached inside the string and assume
   the contents are never mutated. */
#define hmStringGetCString(string) ((const char*)(string)->content)
#define hmStringGetChars(string) ((string)->content)

hm_uint32 hmStringHashFunc(void* key, hm_uint32 salt);
hm_bool hmStringEqualsFunc(void* value1, void* value2);
hmError hmStringDisposeFunc(void* obj);

/* Sometimes containers may need references to strings someone else owns. These hash/equals functions
   allow to operate on string references instead of strings themselves. */
hm_uint32 hmStringRefHashFunc(void* key, hm_uint32 salt);
hm_bool hmStringRefEqualsFunc(void* value1, void* value2);

#endif /* HM_STRING_H */
