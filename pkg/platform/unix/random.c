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
#include <core/hash.h>
#include <threading/thread.h>

#if defined __GLIBC__ && defined __linux__
    #if __GLIBC__ > 2 || __GLIBC_MINOR__ > 24
        #define HM_SUPPORTS_GET_RANDOM
    #endif
#endif

#ifdef HM_SUPPORTS_GET_RANDOM
    #include <sys/random.h>
#endif

hm_int32 hmGenerateSeed()
{
    hm_int32 ret_value = 0;
#ifdef HM_SUPPORTS_GET_RANDOM
    ssize_t result = getrandom(&ret_value, sizeof(ret_value), 0);
    if (result == -1) {
#endif
        /* Falls back on the current time if /dev/urandom is not available.
           To make it more unpredictable, the current time is additionally hashed. */
        hm_millis tick_count = hmGetTickCount();
        hm_uint32 tick_count_hash = hmHash(&tick_count, sizeof(tick_count), (hm_uint32)tick_count);
        ret_value = *((hm_int32*)(&tick_count_hash)); /* type-punning to convert uint32 to int32 bitwise */
        /* Makes it more likely that each new seed is different. Otherwise, if hmGenerateSeed was called repeatedly,
           it would be possible to generate same seeds when based on the current tick count. */
        hmSleep(16);
#ifdef HM_SUPPORTS_GET_RANDOM
    }
#endif
    return ret_value;
}
