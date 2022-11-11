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

#include <threading/mutex.h>
#include <core/allocator.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t posix_mutex;
} hmMutexPlatformData;

#define hmResultToError(result) ((result) ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK)
#define hmMutexGetPosixMutexRef(mutex) &((hmMutexPlatformData*)(mutex)->platform_data)->posix_mutex
#define HM_TRY_OR_FINALIZE_FOR_RESULT(err, expr) HM_TRY_OR_FINALIZE(err, hmResultToError(expr))

hmError hmCreateMutex(hmAllocator* allocator, hmMutex* in_mutex)
{
    hmMutexPlatformData* platform_data = (hmMutexPlatformData*)hmAlloc(allocator, sizeof(hmMutexPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    pthread_mutexattr_t mutex_attr;
    HM_TRY_OR_FINALIZE_FOR_RESULT(err, pthread_mutexattr_init(&mutex_attr));
    HM_TRY_OR_FINALIZE_FOR_RESULT(err, pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE));
    HM_TRY_OR_FINALIZE_FOR_RESULT(err, pthread_mutex_init(&platform_data->posix_mutex, &mutex_attr));
    in_mutex->allocator = allocator;
    in_mutex->platform_data = platform_data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmMutexLock(hmMutex* mutex)
{
    return hmResultToError(pthread_mutex_lock(hmMutexGetPosixMutexRef(mutex)));
}

hmError hmMutexUnlock(hmMutex* mutex)
{
    return hmResultToError(pthread_mutex_unlock(hmMutexGetPosixMutexRef(mutex)));
}

hmError hmMutexDispose(hmMutex* mutex)
{
    hmError err = hmResultToError(pthread_mutex_destroy(hmMutexGetPosixMutexRef(mutex)));
    hmFree(mutex->allocator, mutex->platform_data);
    return err;
}
