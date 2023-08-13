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

#include <platform/unix/common.h>
#include <core/math.h>

#include <errno.h> /* for error codes */
#include <time.h>  /* for clock_gettime(..), CLOCK_MONOTONIC and CLOCK_REALTIME */

hmError hmUnixErrorToHammer(int unix_err)
{
    switch (unix_err) {
        case HM_UNIX_OK:
            return HM_OK;
        case ETIMEDOUT:
            return HM_ERROR_TIMEOUT;
        case ENETUNREACH:
            return HM_ERROR_NOT_FOUND;
        case ECONNREFUSED:
        case EADDRINUSE:
            return HM_ERROR_ACCESS_DENIED;
        default:
            return HM_ERROR_PLATFORM_DEPENDENT;
    }
}

struct timespec hmConvertMillisecondsToTimeSpec(hm_millis ms)
{
    struct timespec ts = {0};
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    return ts;
}

hm_millis hmConvertTimeSpecToMilliseconds(struct timespec* ts)
{
    return (hm_millis)((ts->tv_sec * 1000) + (ts->tv_nsec / (1000 * 1000)));
}

struct timeval hmConvertMillisecondsToTimeVal(hm_millis ms)
{
    struct timeval tv = {0};
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    return tv;
}

struct timespec hmGetCurrentTimeSpec(hm_bool is_monotonic)
{
    struct timespec ts = {0};
    clock_gettime(is_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, &ts);
    return ts;
}

hmError hmGetFutureTimeSpec(hm_bool is_monotonic, hm_millis ms_in_future, struct timespec* in_timespec)
{
    struct timespec ts = hmGetCurrentTimeSpec(is_monotonic);
    hm_millis ms = hmConvertTimeSpecToMilliseconds(&ts);
    hm_millis new_ms = 0;
    HM_TRY(hmAddMillis(ms, ms_in_future, &new_ms));
    *in_timespec = hmConvertMillisecondsToTimeSpec(new_ms);
    return HM_OK;
}
