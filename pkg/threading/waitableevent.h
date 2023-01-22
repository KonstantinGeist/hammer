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

#ifndef HM_WAITABLE_EVENT_H
#define HM_WAITABLE_EVENT_H

#include <core/common.h>
#include <core/allocator.h>

#define HM_WAITABLE_EVENT_MIN_TIMEOUT_MS 1
#define HM_WAITABLE_EVENT_MAX_TIMEOUT_MS (60*60*1000) /* 1 hour must be more than enough */

typedef struct {
    hmAllocator* allocator;
    void*        platform_data; /* Platform-specific data are hidden from header files.
                                   Also a pointer guards against moves/copies. */
} hmWaitableEvent;

/* Creates a "waitable event" which allows to block the current thread until the waitable event's Signal() method is called.
   Useful for building queue consumers to avoid burning the CPU while waiting. */
hmError hmCreateWaitableEvent(hmAllocator* allocator, hmWaitableEvent* in_waitable_event);
hmError hmWaitableEventDispose(hmWaitableEvent* waitable_event);
/* Blocks the current thread until the waitable event is "signaled" (gets Signal() called) or the interval `timeout_ms`
   (in milliseconds) elapses.
   Returns HM_OK if the current thread was woken up via Signal(); returns HM_ERROR_TIMEOUT if the timeout expired.
   `timeout_ms` is restricted to the range from HM_WAITABLE_EVENT_MIN_TIMEOUT_MS to HM_WAITABLE_EVENT_MAX_TIMEOUT_MS (otherwise,
   HM_ERROR_INVALID_ARGUMENT is returned). This way, we don't have to deal with corner cases (zero or infinite timeouts).
 */
hmError hmWaitableEventWait(hmWaitableEvent* waitable_event, hm_millis timeout_ms);
/* Allows one waiting thread to proceed. Only one thread at a time is guaranteed to proceed.
   After a waitable event is signaled, any new threads calling hmWaitableEventWait(..) will block again.  */
hmError hmWaitableEventSignal(hmWaitableEvent* waitable_event);

#endif /* HM_WAITABLE_EVENT_H */
