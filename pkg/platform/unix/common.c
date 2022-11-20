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

#include <platform/unix/common.h>

#include <time.h>

struct timespec hmConvertMillisecondsToTimeSpec(hm_nint ms)
{
    struct timespec ts;
    ts.tv_sec = (uint64_t)ms / 1000;
    ts.tv_nsec = ((uint64_t)ms % 1000) * 1000 * 1000;
    return ts;
}

hm_nint hmConvertTimeSpecToMilliseconds(struct timespec* ts)
{
    return (ts->tv_sec * 1000) + (ts->tv_nsec / (1000 * 1000));
}

struct timespec hmGetCurrentTimeSpec(hm_bool is_monotonic)
{
    struct timespec ts;
    clock_gettime(is_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, &ts);
    return ts;
}

struct timespec hmGetFutureTimeSpec(hm_bool is_monotonic, hm_nint ms_in_future)
{
    struct timespec ts = hmGetCurrentTimeSpec(is_monotonic);
    hm_nint ms = hmConvertTimeSpecToMilliseconds(&ts);
    ms += ms_in_future;
    return hmConvertMillisecondsToTimeSpec(ms);
}
