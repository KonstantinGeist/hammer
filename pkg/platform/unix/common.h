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

struct timespec hmConvertMillisecondsToTimeSpec(hm_millis ms);
hm_millis hmConvertTimeSpecToMilliseconds(struct timespec* ts);
/* Returns the current time as a timespec struct. If is_monotonic is HM_TRUE, returns monotonic time;
   otherwise, returns real time. */
struct timespec hmGetCurrentTimeSpec(hm_bool is_monotonic);
/* Returns a point in time which equals to now + ms_in_future. If is_monotonic is HM_TRUE, returns monotonic time;
   otherwise, returns real time */
hmError hmGetFutureTimeSpec(hm_bool is_monotonic, hm_millis ms_in_future, struct timespec* in_timespec);

#endif /* HM_PLATFORM_COMMON_H */
