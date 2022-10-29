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

#ifndef HM_COMMON_H
#define HM_COMMON_H

/* This header file is included by every source file as it contains all the necessary bits to write C code in the
   idiom we prefer. */

#ifndef NDEBUG
    #define HM_DEBUG
#endif

#define _GNU_SOURCE /* without this define, sqlite3 breaks with stdint for some reason */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* A readable constant for null pointers. */
#define HM_NULL ((void*)0)

/* Platform-specific integer size. */
typedef size_t hm_nint;
typedef uint8_t hm_uint8;
typedef uint32_t hm_uint32;

typedef uint8_t hm_bool;
#define HM_TRUE ((hm_bool)1)
#define HM_FALSE ((hm_bool)0)

typedef double hm_float;

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
#define HM_ERROR_NOT_FOUND          ((hmError)5) /* Resource (for example, a file, or an element in a hashmap) was not found. */
#define HM_ERROR_PLATFORM_DEPENDENT ((hmError)6) /* A platform-dependent error occurred. */
#define HM_ERROR_INVALID_IMAGE      ((hmError)7) /* Invalid binary data found. */

/* Allows to combine several errors into one. Usually useful when a new error occurs while processing another error. */
hmError hmCombineErrors(hmError older, hmError newer);

/* A generic function which is able to dispose of an object. Used in containers to automatically delete items
   on container destruction. */
typedef hmError (*hmDisposeFunc)(void* object);

/* Special attributes to extend C semantics with some human-readable metadata about how memory is managed. */

/* The variable is temporary and should be deallocated, unless its ownership is moved somewhere else. */
#define HM_TEMP_SHOULD_DEALLOC(var)
/* The variable is temporary and it should not be deallocated, because it's a view; however, as a view,
   its content may get invalidated when the original object is invalidated. */
#define HM_TEMP_VIEW(var)
/* The temporary variable was successfully moved because its content's ownership was transferred to another
   entity; you don't need to deallocated it anymore if it was a temporary; may be problematic with views. */
#define HM_MOVED(from, to)

/* A handy macro which checks the error as returned by the given expression and immediately returns if it's not HM_OK. */
#define HM_TRY(expr) hmError err ## __LINE__ = expr; if (err ## __LINE__ != HM_OK) return err ## __LINE__

#endif /* HM_COMMON_H */
