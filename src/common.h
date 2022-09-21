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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* This header file is included by every source file as it contains all the necessary bits to write C code in the
   idiom we prefer. */

/* A readable constant for null pointers. */
#define HM_NULL ((void*)0)

/* Platform-specific integer size. */
typedef size_t hm_nint;
/* 8-bit integer. */
typedef uint8_t hm_uint8;

/* Almost all functions are expected to return an error value. If no error happened, HM_OK should be returned.
   Return function-specific data as the last function argument passed by reference. */
typedef hm_uint8 hmError;
#define HM_OK                     ((hmError)0) /* No error happened. */
#define HM_ERROR_OUT_OF_MEMORY    ((hmError)1) /* The allocator ran out of memory. */
#define HM_ERROR_INVALID_ARGUMENT ((hmError)2) /* An invalid argument was passed to a function. As we strive to be zero-downtime,
                                                  each function in the runtime should carefully check all its argument to avoid
                                                  crashing the whole process (at the expense of some slowdown). */
#define HM_ERROR_INVALID_STATE    ((hmError)3) /* A function is called on an object which is not in the required state. */

#endif /* HM_COMMON_H */
