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

struct timespec hmConvertMillisecondsToTimeSpec(hm_nint milliseconds)
{
    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t nanoseconds = ((uint64_t) tv.tv_sec) * 1000 * 1000 * 1000
              + (uint64_t)milliseconds * 1000 * 1000 + ((uint64_t) tv.tv_usec) * 1000;
    ts.tv_sec = nanoseconds / 1000 / 1000 / 1000;
    ts.tv_nsec = (nanoseconds - ((uint64_t) ts.tv_sec) * 1000 * 1000 * 1000);
    return ts;
}
