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
    char*        content;         /* The actual string content. If the `allocator` is specified, the content is owned by the string
                                    (and disposed in hmStringDispose(..) Otherwise, it's just a view string. */
    hmAllocator* allocator_opt;   /* See `content` above on the implications of this field's optionality. */
    hm_nint      length_in_bytes; /* String's length in bytes is remembered to avoid O(n) lookups every time we need a string's length. */
} hmString;

/* Creates a Hammer string from a null-terminated C string. Duplicates the given string and owns it: deallocates the
   internal buffer when the object is disposed of. See also hmCreateStringViewFromCString.
   Strings are generally immutable. The encoding is expected to be UTF8; although it's not enforced in this constructor,
   certain functions such as hmStringIndexRune(..) do check that it's a valid UTF8 string. */
hmError hmCreateStringFromCString(hmAllocator* allocator, const char* content, hmString* in_string);
/* Same as hmCreateStringFromCString(..) except it doesn't rely on null termination -- instead, the length is provided
   as an argument (the null terminator is not included in the length).
   It's the responsibility of the caller to make sure there's no buffer overflow if length parameter is larger than
   the actual string. Empty strings with zero length are allowed.
   Strings are generally immutable. The encoding is expected to be UTF8; although it's not enforced in this constructor,
   certain functions such as hmStringIndexRune(..) do check that it's a valid UTF8 string. */
hmError hmCreateStringFromCStringWithLengthInBytes(hmAllocator* allocator, const char* content, hm_nint length_in_bytes, hmString* in_string);
/* Creates a Hammer string from a null-terminated C string. Unlike hmCreateStringFromCString (see), does not duplicate
   the string and does not own the internal buffer. The string view will be invalidated after the referenced string
   is deleted; it's undefined behavior to try to use such a string afterwards. Mostly useful for creating short-lived
   views for reading, for example, as a key to a container (if the container promises to never retain the value).
   String views should not be disposed, but it should be safe to try to dispose them.
   Strings are generally immutable. */
hmError hmCreateStringViewFromCString(const char* content, hmString* in_string);
/* Creates a substring from the given Hammer string `source`, starting from `start_index` and ending with `start + length_in_bytes`. */
hmError hmCreateSubstring(hmAllocator* allocator, hmString* source, hm_nint start_index, hm_nint length_in_bytes, hmString* in_string);
/* Creates an empty string view. Same as hmCreateStringViewFromCString("", ..)
   Strings are generally immutable. */
hmError hmCreateEmptyStringView(hmString* in_string);
/* Clones the given string as a new instance. */
hmError hmStringDuplicate(hmAllocator* allocator, hmString* string, hmString* in_duplicate);
hmError hmStringDispose(hmString* string);
/* A quick way to compare if then given string contains the given content. */
hm_bool hmStringEqualsToCString(hmString* string, const char* content);
/* Returns true of the string starts with the given C string literal and length.
   Behavior is undefined if `prefix_length` is greater than the actual length of `prefix`. */
hm_bool hmStringStartsWithCStringAndLength(hmString* string, const char* prefix, hm_nint prefix_length);
/* Returns true of the string ends with the given C string literal and length.
   Behavior is undefined if `suffix_length` is greater than the actual length of `suffix`. */
hm_bool hmStringEndsWithCStringAndLength(hmString* string, const char* suffix, hm_nint suffix_length);
/* Returns true of the string starts with the given C string literal. Implemented via hmStringStartsWithCStringAndLengthInBytes(..) */
hm_bool hmStringStartsWithCString(hmString* string, const char* prefix);
/* Returns true of the string ends with the given C string literal. Implemented via hmStringEndsWithCStringAndLengthInBytes(..) */
hm_bool hmStringEndsWithCString(hmString* string, const char* suffix);
/* Compares two Hammer strings for equality. */
hm_bool hmStringEquals(hmString* string1, hmString* string2);
/* Hashes a string. For `salt`, see hmHash(..) */
hm_uint32 hmStringHash(hmString* string, hm_uint32 salt);
/* Returns the length of the string in bytes. The length may be computed lazily and is cached inside the string. */
hm_nint hmStringGetLengthInBytes(hmString* string);
/* Returns HM_TRUE if the string is "empty", i.e. contains zero characters. Same as `hmStringGetLengthInBytes(string) == 0`,
   just more readable. */
#define hmStringIsEmpty(string) (hmStringGetLengthInBytes(string) == 0)
/* Returns the raw contents of the string as a null-terminated C string. The contents should stay immutable
   because certain values, such as the string's length, can be cached inside the string and assume
   the contents are never mutated. Use this function to pass the buffer to foreign code which supports C ABI.
   See also: hmStringGetChars(..), hmStringGetCharsForUpdate(..) */
#define hmStringGetCString(string) ((const char*)(string)->content)
/* Returns the internal char array of the string for quicker read-only access to the underlying data.
   See also: hmStringGetCString(..), hmStringGetCharsForUpdate(..) */
#define hmStringGetChars(string) ((string)->content)
/* Returns the internal char array in `out_buffer` for in-place updates. If the string is a read-only view, returns HM_ERROR_INVALID_STATE.
   Supports trimming the original char buffer with '\0' in the middle: string length will be recalculated in that case.
   WARNING: don't update string content for strings used as keys to hashmaps etc.
   See also: hmStringGetCString(..), hmStringGetChars(..) */
hmError hmStringGetCharsForUpdate(hmString* string, char** out_chars);
/* The comparison function of strings. Useful in hmArraySort(..) */
hmComparisonResult hmStringCompare(hmString* string1, hmString* string2);
/* Returns the index (offset into the byte array) of the given rune in `out_index_opt`.
   If the rune is not found, returns HM_ERROR_NOT_FOUND.
   If the string is not a well-formed UTF8 string, returns HM_ERROR_INVALID_DATA.
   Parameter `out_index_opt` can be null for cases when we just want to see if the rune is present in the string. */
hmError hmStringIndexRune(hmString* string, hm_rune rune_to_index, hm_nint* out_index_opt);

hm_uint32 hmStringHashFunc(void* key, hm_uint32 salt);
hm_bool hmStringEqualsFunc(void* value1, void* value2);
hmError hmStringDisposeFunc(void* obj);
hmComparisonResult hmStringCompareFunc(void* value1, void* value2, void* user_data);

/* Sometimes containers may need references to strings someone else owns. These hash/equals functions
   allow to operate on string references instead of strings themselves. */
hm_uint32 hmStringRefHashFunc(void* key, hm_uint32 salt);
hm_bool hmStringRefEqualsFunc(void* value1, void* value2);

#endif /* HM_STRING_H */
