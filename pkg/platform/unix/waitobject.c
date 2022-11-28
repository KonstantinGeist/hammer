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

#include <threading/waitobject.h>
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
} hmWaitObjectPlatformData;

#define hmWaitObjectGetPlatformData(wait_object) ((hmWaitObjectPlatformData*)(wait_object)->platform_data)
static hmError hmWaitObjectWaitWithoutLock(hmWaitObjectPlatformData* platform_data, hm_millis timeout);

hmError hmCreateWaitObject(hmAllocator* allocator, hmWaitObject* in_wait_object)
{
    hmWaitObjectPlatformData* platform_data = (hmWaitObjectPlatformData*)hmAlloc(allocator, sizeof(hmWaitObjectPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmResultToError(pthread_cond_init(&platform_data->cond_variable, HM_NULL)));
    HM_TRY_OR_FINALIZE(err, hmResultToError(pthread_mutex_init(&platform_data->mutex, HM_NULL)));
    hmAtomicStore(&platform_data->signaled_state, HM_FALSE);
    in_wait_object->allocator = allocator;
    in_wait_object->platform_data = platform_data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmWaitObjectDispose(hmWaitObject* wait_object)
{
    hmError err = HM_OK;
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    err = hmCombineErrors(err, hmResultToError(pthread_cond_destroy(&platform_data->cond_variable)));
    err = hmCombineErrors(err, hmResultToError(pthread_mutex_destroy(&platform_data->mutex)));
    hmFree(wait_object->allocator, platform_data);
    return err;
}

hmError hmWaitObjectWait(hmWaitObject* wait_object, hm_millis timeout_ms)
{
    if (timeout_ms < HM_WAIT_OBJECT_MIN_TIMEOUT_MS || timeout_ms > HM_WAIT_OBJECT_MAX_TIMEOUT_MS) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    HM_TRY_FOR_RESULT(pthread_mutex_lock(&platform_data->mutex));
    hmError err = hmWaitObjectWaitWithoutLock(platform_data, timeout_ms);
    return hmCombineErrors(err, hmResultToError(pthread_mutex_unlock(&platform_data->mutex))); /* always unlock */
}

hmError hmWaitObjectPulse(hmWaitObject* wait_object)
{
    /* The classic idiom: the state is protected with a mutex + then there's a call to pthread_cond_signal(..) which unblocks
       pthread_cond_timedwait(..) in hmWaitObjectWaitWithoutLock, allowing a blocked consumer to proceed. */
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    HM_TRY_FOR_RESULT(pthread_mutex_lock(&platform_data->mutex));
    hmAtomicStore(&platform_data->signaled_state, HM_TRUE);
    HM_TRY_FOR_RESULT(pthread_mutex_unlock(&platform_data->mutex));
    HM_TRY_FOR_RESULT(pthread_cond_signal(&platform_data->cond_variable));
    return HM_OK;
}

static hmError hmWaitObjectWaitWithoutLock(hmWaitObjectPlatformData* platform_data, hm_millis timeout_ms)
{
    int result = POSIX_RESULT_OK;
    if (!hmAtomicLoad(&platform_data->signaled_state)) {
        struct timespec ts;
        HM_TRY(hmGetFutureTimeSpec(HM_FALSE, timeout_ms, &ts));
        do {
            result = pthread_cond_timedwait(&platform_data->cond_variable, &platform_data->mutex, &ts);
        } while (result == POSIX_RESULT_OK && !hmAtomicLoad(&platform_data->signaled_state)); /* a check to protect against spurious wakeups */
    }
    if (result == POSIX_RESULT_OK) {
        hmAtomicStore(&platform_data->signaled_state, HM_FALSE); /* Resets the state back to "non-signaled". */
    }
    return result == ETIMEDOUT ? HM_ERROR_TIMEOUT : hmResultToError(result);
}
