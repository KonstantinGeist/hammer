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

#include <threading/waitobject.h>
#include <core/allocator.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond_variable;
    hm_bool auto_reset;
    hm_bool state;
} hmWaitObjectPlatformData;

#define hmResultToError(result) ((result) ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK)
#define hmWaitObjectGetPlatformData(wait_object) ((hmWaitObjectPlatformData*)(wait_object)->platform_data)
#define HM_TRY_OR_FINALIZE_FOR_RESULT(err, expr) HM_TRY_OR_FINALIZE(err, hmResultToError(expr))
#define HM_TRY_FOR_RESULT(expr) HM_TRY(hmResultToError(expr))

static hmError hmWaitObjectSetEvent(hmWaitObject* wait_object);
static hmError hmWaitObjectResetEvent(hmWaitObject* wait_object);
static hmError hmWaitObjectUnlockedWaitForEvent(hmWaitObject* wait_object, hm_nint timeout);

hmError hmCreateWaitObject(
    hmAllocator* allocator,
    hmWaitObject* in_wait_object,
    hm_bool initial_state,
    hm_bool auto_reset
)
{
    hmWaitObjectPlatformData* platform_data = (hmWaitObjectPlatformData*)hmAlloc(allocator, sizeof(hmWaitObjectPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE_FOR_RESULT(err, pthread_cond_init(&platform_data->cond_variable, 0));
    HM_TRY_OR_FINALIZE_FOR_RESULT(err, pthread_mutex_init(&platform_data->mutex, 0));
    platform_data->state = HM_TRUE;
    platform_data->auto_reset = auto_reset;
    in_wait_object->allocator = allocator;
    in_wait_object->platform_data = platform_data;
    if (initial_state) {
        HM_TRY_OR_FINALIZE(err, hmWaitObjectSetEvent(in_wait_object));
    }
HM_ON_FINALIZE
    if (err != HM_OK) {
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmWaitObjectWait(hmWaitObject* wait_object, hm_nint timeout)
{
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    if (timeout == 0) {
        int result = pthread_mutex_trylock(&platform_data->mutex);
        if (result == EBUSY) {
            return HM_ERROR_TIMEOUT;
        }
        HM_TRY_FOR_RESULT(result);
    } else {
        HM_TRY_FOR_RESULT(pthread_mutex_lock(&platform_data->mutex));
    }
    hmError err = hmWaitObjectUnlockedWaitForEvent(wait_object, timeout);
    HM_TRY(pthread_mutex_unlock(&platform_data->mutex));
    return err;
}

hmError hmWaitObjectPulse(hmWaitObject* wait_object)
{
    HM_TRY(hmWaitObjectSetEvent(wait_object));
    return hmWaitObjectResetEvent(wait_object);
}

hmError hmWaitObjectDispose(hmWaitObject* wait_object)
{
    hmError err = HM_OK;
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    HM_TRY_OR_FINALIZE_FOR_RESULT(err, pthread_cond_destroy(&platform_data->cond_variable));
    HM_TRY_OR_FINALIZE_FOR_RESULT(err, pthread_mutex_destroy(&platform_data->mutex));
HM_ON_FINALIZE
    hmFree(wait_object->allocator, platform_data);
    return err;
}

static hmError hmWaitObjectSetEvent(hmWaitObject* wait_object)
{
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    HM_TRY_FOR_RESULT(pthread_mutex_lock(&platform_data->mutex));
    platform_data->state = HM_TRUE;
    if (platform_data->auto_reset) {
        if (platform_data->state) {
            HM_TRY_FOR_RESULT(pthread_mutex_unlock(&platform_data->mutex));
            HM_TRY_FOR_RESULT(pthread_cond_signal(&platform_data->cond_variable));
            return HM_OK;
        }
    } else {
        HM_TRY_FOR_RESULT(pthread_mutex_unlock(&platform_data->mutex));
        HM_TRY_FOR_RESULT(pthread_cond_broadcast(&platform_data->cond_variable));
    }
    return HM_OK;
}

static hmError hmWaitObjectResetEvent(hmWaitObject* wait_object)
{
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    HM_TRY_FOR_RESULT(pthread_mutex_lock(&platform_data->mutex));
    platform_data->state = HM_FALSE;
    HM_TRY_FOR_RESULT(pthread_mutex_unlock(&platform_data->mutex));
    return HM_OK;
}

static hmError hmWaitObjectUnlockedWaitForEvent(hmWaitObject* wait_object, hm_nint _timeout)
{
    uint64_t timeout = (uint64_t)_timeout;
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    int result = 0;
    if (!platform_data->state) {
        if (timeout == 0) {
            return HM_ERROR_TIMEOUT;
        }
        struct timespec ts;
        if (timeout != (uint64_t) -1) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            uint64_t nanoseconds = ((uint64_t) tv.tv_sec) * 1000 * 1000 * 1000
                      + timeout * 1000 * 1000 + ((uint64_t) tv.tv_usec) * 1000;
            ts.tv_sec = nanoseconds / 1000 / 1000 / 1000;
            ts.tv_nsec = (nanoseconds - ((uint64_t) ts.tv_sec) * 1000 * 1000 * 1000);
        }
        do {
            if (timeout != (uint64_t) -1) {
                result = pthread_cond_timedwait(&platform_data->cond_variable, &platform_data->mutex, &ts);
            } else {
                result = pthread_cond_wait(&platform_data->cond_variable, &platform_data->mutex);
            }
        } while (result == 0 && !platform_data->state);
        if (result == 0 && platform_data->auto_reset) {
            platform_data->state = HM_FALSE;
        }
    } else if (platform_data->auto_reset) {
        result = 0;
        platform_data->state = HM_FALSE;
    }
    return result == ETIMEDOUT ? HM_ERROR_TIMEOUT : HM_OK;
}
