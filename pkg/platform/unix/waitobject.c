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

/*
 * Based on:
 *      WIN32 Events for POSIX
 *      Author: Mahmoud Al-Qudsi <mqudsi@neosmart.net>
 *      Copyright (C) 2011 - 2019 by NeoSmart Technologies
 *      MIT License
 */

#include <threading/waitobject.h>
#include <core/allocator.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond_variable;
    hm_bool         signaled_state; /* access to be protected with the mutex */
} hmWaitObjectPlatformData;

#define POSIX_RESULT_OK 0

#define hmResultToError(result) ((result) != POSIX_RESULT_OK ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK)
#define hmWaitObjectGetPlatformData(wait_object) ((hmWaitObjectPlatformData*)(wait_object)->platform_data)
#define HM_TRY_FOR_RESULT(expr) HM_TRY(hmResultToError(expr))

static hmError hmWaitObjectSignalEvent(hmWaitObjectPlatformData* platform_data);
static hmError hmWaitObjectResetEvent(hmWaitObjectPlatformData* platform_data);
static hmError hmWaitObjectWaitWithoutLock(hmWaitObjectPlatformData* platform_data, hm_nint timeout);

hmError hmCreateWaitObject(hmAllocator* allocator, hmWaitObject* in_wait_object)
{
    hmWaitObjectPlatformData* platform_data = (hmWaitObjectPlatformData*)hmAlloc(allocator, sizeof(hmWaitObjectPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmResultToError(pthread_cond_init(&platform_data->cond_variable, 0)));
    HM_TRY_OR_FINALIZE(err, hmResultToError(pthread_mutex_init(&platform_data->mutex, 0)));
    platform_data->signaled_state = HM_FALSE;
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

hmError hmWaitObjectWait(hmWaitObject* wait_object, hm_nint timeout_ms)
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
    hmWaitObjectPlatformData* platform_data = hmWaitObjectGetPlatformData(wait_object);
    HM_TRY(hmWaitObjectSignalEvent(platform_data));
    return hmWaitObjectResetEvent(platform_data);
}

static hmError hmWaitObjectSignalEvent(hmWaitObjectPlatformData* platform_data)
{
    HM_TRY_FOR_RESULT(pthread_mutex_lock(&platform_data->mutex));
    platform_data->signaled_state = HM_TRUE;
    HM_TRY_FOR_RESULT(pthread_mutex_unlock(&platform_data->mutex));
    HM_TRY_FOR_RESULT(pthread_cond_signal(&platform_data->cond_variable));
    return HM_OK;
}

static hmError hmWaitObjectResetEvent(hmWaitObjectPlatformData* platform_data)
{
    HM_TRY_FOR_RESULT(pthread_mutex_lock(&platform_data->mutex));
    platform_data->signaled_state = HM_FALSE;
    HM_TRY_FOR_RESULT(pthread_mutex_unlock(&platform_data->mutex));
    return HM_OK;
}

static struct timespec convert_timeout_ms_to_timespec(hm_nint timeout_ms)
{
    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t nanoseconds = ((uint64_t) tv.tv_sec) * 1000 * 1000 * 1000
              + (uint64_t)timeout_ms * 1000 * 1000 + ((uint64_t) tv.tv_usec) * 1000;
    ts.tv_sec = nanoseconds / 1000 / 1000 / 1000;
    ts.tv_nsec = (nanoseconds - ((uint64_t) ts.tv_sec) * 1000 * 1000 * 1000);
    return ts;
}

static hmError hmWaitObjectWaitWithoutLock(hmWaitObjectPlatformData* platform_data, hm_nint timeout_ms)
{
    if (platform_data->signaled_state) {
        platform_data->signaled_state = HM_FALSE;
        return HM_OK;
    }
    int result = POSIX_RESULT_OK;
    struct timespec ts = convert_timeout_ms_to_timespec(timeout_ms);
    do {
        result = pthread_cond_timedwait(&platform_data->cond_variable, &platform_data->mutex, &ts);
    } while (result == POSIX_RESULT_OK && !platform_data->signaled_state); /* a check to protect against spurious wakeups */
    if (result == POSIX_RESULT_OK) {
        platform_data->signaled_state = HM_FALSE;
    }
    return result == ETIMEDOUT ? HM_ERROR_TIMEOUT : HM_OK;
}
