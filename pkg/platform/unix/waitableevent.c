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

/*
 * Based on:
 *      WIN32 Events for POSIX
 *      Author: Mahmoud Al-Qudsi <mqudsi@neosmart.net>
 *      Copyright (C) 2011 - 2019 by NeoSmart Technologies
 *      MIT License
 */

#include <threading/waitableevent.h>
#include <threading/atomic.h>
#include <core/math.h>
#include <platform/unix/common.h>

#include <core/allocator.h>
#include <errno.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t         mutex;
    pthread_cond_t          cond_variable;
    volatile hm_atomic_nint signaled_state; /* access to be protected with the mutex */
} hmWaitableEventPlatformData;

#define hmWaitableEventGetPlatformData(waitable_event) ((hmWaitableEventPlatformData*)(waitable_event)->platform_data)
static hmError hmWaitableEventWaitWithoutLock(hmWaitableEventPlatformData* platform_data, hm_millis timeout);

hmError hmCreateWaitableEvent(hmAllocator* allocator, hmWaitableEvent* in_waitable_event)
{
    hmWaitableEventPlatformData* platform_data = (hmWaitableEventPlatformData*)hmAlloc(allocator, sizeof(hmWaitableEventPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmUnixErrorToHammer(pthread_cond_init(&platform_data->cond_variable, HM_NULL)));
    HM_TRY_OR_FINALIZE(err, hmUnixErrorToHammer(pthread_mutex_init(&platform_data->mutex, HM_NULL)));
    hmAtomicStore(&platform_data->signaled_state, HM_FALSE);
    in_waitable_event->allocator = allocator;
    in_waitable_event->platform_data = platform_data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmWaitableEventDispose(hmWaitableEvent* waitable_event)
{
    hmError err = HM_OK;
    hmWaitableEventPlatformData* platform_data = hmWaitableEventGetPlatformData(waitable_event);
    err = hmMergeErrors(err, hmUnixErrorToHammer(pthread_cond_destroy(&platform_data->cond_variable)));
    err = hmMergeErrors(err, hmUnixErrorToHammer(pthread_mutex_destroy(&platform_data->mutex)));
    hmFree(waitable_event->allocator, platform_data);
    return err;
}

hmError hmWaitableEventWait(hmWaitableEvent* waitable_event, hm_millis timeout_ms)
{
    if (timeout_ms < HM_WAITABLE_EVENT_MIN_TIMEOUT_MS || timeout_ms > HM_WAITABLE_EVENT_MAX_TIMEOUT_MS) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmWaitableEventPlatformData* platform_data = hmWaitableEventGetPlatformData(waitable_event);
    HM_TRY_FOR_UNIX_ERROR(pthread_mutex_lock(&platform_data->mutex));
    hmError err = hmWaitableEventWaitWithoutLock(platform_data, timeout_ms);
    return hmMergeErrors(err, hmUnixErrorToHammer(pthread_mutex_unlock(&platform_data->mutex))); /* always unlock */
}

hmError hmWaitableEventSignal(hmWaitableEvent* waitable_event)
{
    /* The classic idiom: the state is protected with a mutex + then there's a call to pthread_cond_signal(..) which unblocks
       pthread_cond_timedwait(..) in hmWaitableEventWaitWithoutLock, allowing a blocked consumer to proceed. */
    hmWaitableEventPlatformData* platform_data = hmWaitableEventGetPlatformData(waitable_event);
    HM_TRY_FOR_UNIX_ERROR(pthread_mutex_lock(&platform_data->mutex));
    hmAtomicStore(&platform_data->signaled_state, HM_TRUE);
    HM_TRY_FOR_UNIX_ERROR(pthread_mutex_unlock(&platform_data->mutex));
    HM_TRY_FOR_UNIX_ERROR(pthread_cond_signal(&platform_data->cond_variable));
    return HM_OK;
}

static hmError hmWaitableEventWaitWithoutLock(hmWaitableEventPlatformData* platform_data, hm_millis timeout_ms)
{
    int unix_err = HM_UNIX_OK;
    if (!hmAtomicLoad(&platform_data->signaled_state)) {
        struct timespec ts;
        HM_TRY(hmGetFutureTimeSpec(HM_FALSE, timeout_ms, &ts));
        do {
            unix_err = pthread_cond_timedwait(&platform_data->cond_variable, &platform_data->mutex, &ts);
        } while (unix_err == HM_UNIX_OK && !hmAtomicLoad(&platform_data->signaled_state)); /* a check to protect against spurious wakeups */
    }
    if (unix_err == HM_UNIX_OK) {
        hmAtomicStore(&platform_data->signaled_state, HM_FALSE); /* Resets the state back to "non-signaled". */
    }
    return hmUnixErrorToHammer(unix_err);
}
