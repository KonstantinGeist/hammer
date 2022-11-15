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

#ifndef HM_WAIT_OBJECT_H
#define HM_WAIT_OBJECT_H

#include <core/common.h>

struct _hmAllocator;

#define HM_WAIT_OBJECT_MIN_TIMEOUT_MS 1
#define HM_WAIT_OBJECT_MAX_TIMEOUT_MS (24*60*60*1000) /* 24 hours must be more than enough */

typedef struct {
    struct _hmAllocator* allocator;
    void*                platform_data; /* Platform-specific data are hidden from header files.
                                           Also a pointer guards against moves/copies. */
} hmWaitObject;

/* Creates a "wait object" which allows to block the current thread until its Pulse() method is called.
   Useful for building queue consumers to avoid burning the CPU while waiting. */
hmError hmCreateWaitObject(struct _hmAllocator* allocator, hmWaitObject* in_wait_object);
hmError hmWaitObjectDispose(hmWaitObject* wait_object);
/* Blocks the current thread until the wait object receives a signal (gets Pulse() called) or the interval `timeout_ms`
   (in milliseconds) elapses.
   Returns HM_OK if the thread was woken up (via Pulse()); returns HM_ERROR_TIMEOUT if the timeout expired.
   `timeout_ms` is restricted to the range from HM_WAIT_OBJECT_MIN_TIMEOUT_MS to HM_WAIT_OBJECT_MAX_TIMEOUT_MS (otherwise,
   HM_ERROR_INVALID_ARGUMENT is returned). This way, we don't have to deal with corner cases.
 */
hmError hmWaitObjectWait(hmWaitObject* wait_object, hm_nint timeout_ms);
/* Sets the state of the object to "signaled", allowing the waiting thread to proceed. Automatically resets
   to non-signaled once the thread is released. Only one thread at a time is guaranteed to proceed. */
hmError hmWaitObjectPulse(hmWaitObject* wait_object);

#endif /* HM_WAIT_OBJECT_H */
