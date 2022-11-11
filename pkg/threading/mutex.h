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

#ifndef HM_MUTEX_H
#define HM_MUTEX_H

#include <core/common.h>

struct _hmAllocator;

typedef struct {
    struct _hmAllocator* allocator;
    void*                platform_data; /* Platform-specific data are hidden from header files.
                                           Also a pointer guards against moves/copies. */
} hmMutex;

/* Creates a recursive mutex. */
hmError hmCreateMutex(struct _hmAllocator* allocator, hmMutex* in_mutex);
hmError hmMutexLock(hmMutex* mutex);
hmError hmMutexUnlock(hmMutex* mutex);
/* It's undefined behavior when a mutex is destroyed while it's locked. */
hmError hmMutexDispose(hmMutex* mutex);

#endif /* HM_MUTEX_H */
