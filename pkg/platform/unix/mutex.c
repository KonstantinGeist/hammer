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

#define hmMutexGetPosixMutexRef(mutex) &((hmMutexPlatformData*)(mutex)->platform_data)->posix_mutex

hmError hmCreateMutex(struct _hmAllocator* allocator, hmMutex* in_mutex)
{
    hmMutexPlatformData* platform_data = (hmMutexPlatformData*)hmAlloc(allocator, sizeof(hmMutexPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    int result = pthread_mutex_init(&platform_data->posix_mutex, &mutex_attr);
    if (result) {
        hmFree(allocator, platform_data);
        return HM_ERROR_PLATFORM_DEPENDENT;
    }
    in_mutex->allocator = allocator;
    in_mutex->platform_data = platform_data;
    return HM_OK;
}

hmError hmMutexLock(hmMutex* mutex)
{
    int result = pthread_mutex_lock(hmMutexGetPosixMutexRef(mutex));
    return result ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK;
}

hmError hmMutexUnlock(hmMutex* mutex)
{
    int result = pthread_mutex_unlock(hmMutexGetPosixMutexRef(mutex));
    return result ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK;
}

hmError hmMutexDispose(hmMutex* mutex)
{
    int result = pthread_mutex_destroy(hmMutexGetPosixMutexRef(mutex));
    if (result) {
        return HM_ERROR_PLATFORM_DEPENDENT;
    }
    hmFree(mutex->allocator, mutex->platform_data);
    return HM_OK;
}
