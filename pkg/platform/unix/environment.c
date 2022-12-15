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

#include <core/environment.h>
#include <platform/unix/common.h>

#include <unistd.h> /* for sysconf and _SC_NPROCESSORS_ONLN  */

hm_millis hmGetTickCount()
{
    struct timespec ts = hmGetCurrentTimeSpec(HM_TRUE);
    return hmConvertTimeSpecToMilliseconds(&ts);
}

hm_nint hmGetProcessorCount()
{
#ifdef _SC_NPROCESSORS_ONLN
    /* Technically non-standard, but we'll simply return 1 processor if no such
       name is found. */
    long result = sysconf(_SC_NPROCESSORS_ONLN);
    /* Assume 1 processor if the call to sysconf wasn't successful. */
    return result < 1 ? 1 : (hm_nint)result;
#else
    return 1;
#endif /* _SC_NPROCESSORS_ONLN */
}
