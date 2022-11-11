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

#define HM_DEFAULT_WAIT_OBJECT_INITIAL_STATE HM_FALSE
#define HM_DEFAULT_WAIT_OBJECT_AUTO_RESET HM_TRUE

struct _hmAllocator;

typedef struct {
    struct _hmAllocator* allocator;
    void*                platform_data; /* Platform-specific data are hidden from header files.
                                           Also a pointer guards against moves/copies. */
} hmWaitObject;

/* Creates a "wait object" which allows to block the current thread until its Pulse() method is called.
   Useful for building queue consumers to avoid burning the CPU while waiting. */
hmError hmCreateWaitObject(
    struct _hmAllocator* allocator,
    hmWaitObject* in_wait_object,
    hm_bool initial_state,
    hm_bool auto_reset
);
/* Blocks the current thread until the wait object receives a signal (gets Pulse() called) using an integer `timeout`
   value (in milliseconds) to specify the time interval. The function waits until the object is signaled or
   the interval elapses. If the parameter is 0, the function will return only when the object is signaled.
   Returns HM_OK if the thread was woken up; returns HM_ERROR_TIMEOUT if the timeout expired.
 */
hmError hmWaitObjectWait(hmWaitObject* wait_object, hm_nint timeout);
/* Sets the state of the object to "signaled", allowing the waiting thread to proceed. Automatically resets
   to non-signaled once the thread is released (if auto_reset is set to HM_TRUE).
   Only one thread at a time is guaranteed to proceed. */
hmError hmWaitObjectPulse(hmWaitObject* wait_object);
hmError hmWaitObjectDispose(hmWaitObject* wait_object);

#endif /* HM_WAIT_OBJECT_H */
