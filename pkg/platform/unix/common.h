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

#ifndef HM_PLATFORM_COMMON_H
#define HM_PLATFORM_COMMON_H

#include <core/common.h>

#include <sys/time.h>

/* Unix-specific functions for converting between Hammer and Unix data formats. */

#define POSIX_RESULT_OK 0
#define hmResultToError(result) ((result) != POSIX_RESULT_OK ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK)
#define HM_TRY_FOR_RESULT(expr) HM_TRY(hmResultToError(expr))

struct timespec hmConvertMillisecondsToTimeSpec(hm_nint milliseconds);

#endif /* HM_PLATFORM_COMMON_H */
