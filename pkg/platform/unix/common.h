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

#ifndef HM_PLATFORM_COMMON_H
#define HM_PLATFORM_COMMON_H

#include <core/common.h>

#include <sys/time.h> /* for timespec */

/* Unix-specific functions for converting between Hammer and Unix/Posix data formats. */

#define HM_UNIX_OK 0
#define HM_TRY_FOR_UNIX_ERROR(expr) HM_TRY(hmUnixErrorToHammer(expr))

hmError hmUnixErrorToHammer(int unix_err);

/* Converts milliseconds to POSIX's timespec. Since time_t is platform-dependent, all users, transitively,
   must have a reasonable limit for `ms`, to prevent overflows. For that reason, functions like hmThreadJoin(..) and
   hmWaitableEventWait(..) have clearly defined limits. */
struct timespec hmConvertMillisecondsToTimeSpec(hm_millis ms);
hm_millis hmConvertTimeSpecToMilliseconds(struct timespec* ts);
/* Same as hmConvertMillisecondsToTimeSpec(..), except converts to timeval instead of timespec. */
struct timeval hmConvertMillisecondsToTimeVal(hm_millis ms);
/* Returns the current time as a timespec struct. If is_monotonic is HM_TRUE, returns monotonic time;
   otherwise, returns real time. */
struct timespec hmGetCurrentTimeSpec(hm_bool is_monotonic);
/* Returns a point in time which equals to now + ms_in_future. If is_monotonic is HM_TRUE, returns monotonic time;
   otherwise, returns real time. See hmConvertMillisecondsToTimeSpec(..) for overflow considerations. */
hmError hmGetFutureTimeSpec(hm_bool is_monotonic, hm_millis ms_in_future, struct timespec* in_timespec);

#endif /* HM_PLATFORM_COMMON_H */
