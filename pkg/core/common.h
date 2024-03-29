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

#ifndef HM_COMMON_H
#define HM_COMMON_H

/* This header file is included by every source file as it contains all the necessary bits to write C code in the
   idiom we prefer. */

#ifndef NDEBUG
    #define HM_DEBUG
#endif

#define _GNU_SOURCE /* without this define, sqlite3 breaks with stdint for some reason */
#include <stdint.h> /* for primitive type definitions (uint8, etc.) */

/* A readable constant for null pointers. */
#define HM_NULL ((void*)0)

/* Platform-specific integer size, can also be cast to/from void pointers. */
typedef uintptr_t hm_nint;
typedef uint8_t hm_uint8;
typedef uint16_t hm_uint16;
typedef uint32_t hm_uint32;
typedef int32_t hm_int32;
typedef uint64_t hm_uint64;
typedef uint64_t hm_millis;
/* Represents a Unicode code point (in the 32-bit range). The name follows Go's convention to make it clear
   it's different from the usual "char". */
typedef int32_t hm_rune;

typedef uint8_t hm_bool;
#define HM_TRUE ((hm_bool)1)
#define HM_FALSE ((hm_bool)0)

typedef double hm_float64;

#define HM_UINT16_MAX UINT16_MAX
#define HM_UINT32_MAX UINT32_MAX
#define HM_INT32_MIN INT32_MIN
#define HM_INT32_MAX INT32_MAX
#define HM_NINT_MAX UINTPTR_MAX
#define HM_MILLIS_MAX UINT32_MAX

/* Almost all functions are expected to return an error value. If no error happened, HM_OK should be returned.
   Return function-specific data as the last function argument passed by reference. */
typedef hm_uint8 hmError;
#define HM_OK                       ((hmError)0) /* No error happened. */
#define HM_ERROR_OUT_OF_MEMORY      ((hmError)1) /* The allocator ran out of memory. */
#define HM_ERROR_INVALID_ARGUMENT   ((hmError)2) /* An invalid argument was passed to a function. As we strive to be zero-downtime,
                                                  each function in the runtime should carefully check all its argument to avoid
                                                  crashing the whole process (at the expense of some slowdown). */
#define HM_ERROR_INVALID_STATE      ((hmError)3) /* A function is called on an object which is not in the required state. */
#define HM_ERROR_OUT_OF_RANGE       ((hmError)4) /* An attempt was made to retrieve an item out of its container's range. */
#define HM_ERROR_NOT_FOUND          ((hmError)5) /* Resource (for example, a file, or an item in a hashmap) was not found. */
#define HM_ERROR_PLATFORM_DEPENDENT ((hmError)6) /* A platform-dependent error occurred. */
#define HM_ERROR_INVALID_DATA       ((hmError)7) /* Invalid data: malformed or corrupted. */
#define HM_ERROR_LIMIT_EXCEEDED     ((hmError)8) /* A certain limit was exceeded. */
#define HM_ERROR_TIMEOUT            ((hmError)9) /* An operation timed out. */
#define HM_ERROR_NOT_IMPLEMENTED    ((hmError)10) /* Operation is not implemented for this platform. */
#define HM_ERROR_OVERFLOW           ((hmError)11) /* Overflow happened. */
#define HM_ERROR_UNDERFLOW          ((hmError)12) /* Underflow happened. */
#define HM_ERROR_ACCESS_DENIED      ((hmError)13) /* Access denied for the given resource. */
#define HM_ERROR_DISCONNECTED       ((hmError)14) /* Connection reset/disconnected. */

/* Allows to merge several errors into one. Usually useful when a new error occurs while processing another error. */
hmError hmMergeErrors(hmError older, hmError newer);

/* A generic function which is able to dispose of an object. Used in containers to automatically delete items
   on container destruction. */
typedef hmError (*hmDisposeFunc)(void* object);

/* A handy macro which checks the error as returned by the given expression and immediately returns if it's not HM_OK. */
#define HM_TRY(expr) { hmError try_err = expr; if (try_err != HM_OK) return try_err; }
#define HM_TRY_OR_FINALIZE(err, expr) err = hmMergeErrors(err, expr); if (err != HM_OK) goto finalize
#define HM_ON_FINALIZE finalize: {} /* Curly brackets are for the cases when the label ends up right before a variable declaration. */
#define HM_FINALIZE goto finalize

/* A few macros to cast nints => pointers and back, with some extra caution in regards to the C Standard. */
#define hmCastPointerToNint(value) ((hm_nint)(void*)value)
#define hmCastNintToPointer(value, type) ((type)(void*)value)

/* Comparison function and its enum values. Useful in situations where it's required to compare two values; for example,
   hmArraySort(..) makes use of it. */
typedef hm_uint8 hmComparisonResult;
#define HM_COMPARISON_RESULT_LESS ((hmComparisonResult)-1)   /* The first value is less than the second value (see hmComparisonResult). */
#define HM_COMPARISON_RESULT_EQUAL ((hmComparisonResult)0)   /* The first value is equal to the second value (see hmComparisonResult). */
#define HM_COMPARISON_RESULT_GREATER ((hmComparisonResult)1) /* The first value is greater than the second value (see hmComparisonResult). */
typedef hmComparisonResult (*hmCompareFunc)(void* value1, void* value2, void* user_data);

#endif /* HM_COMMON_H */
