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

#ifndef HM_THREAD_H
#define HM_THREAD_H

#include <core/common.h>
#include <core/string.h>
#include <threading/atomic.h>

struct _hmAllocator;
struct _hmArray;
struct _hmThread;

#define HM_SLEEP_MIN_MS 1
#define HM_SLEEP_MAX_MS (60*60*1000) /* 1 hour must be more than enough */
#define HM_THREAD_JOIN_MIN_TIMEOUT_MS HM_SLEEP_MIN_MS
#define HM_THREAD_JOIN_MAX_TIMEOUT_MS HM_SLEEP_MAX_MS

typedef hm_atomic_nint hmThreadState;
#define HM_THREAD_STATE_UNSTARTED       ((hmThreadState)0)
#define HM_THREAD_STATE_RUNNING         ((hmThreadState)1)
#define HM_THREAD_STATE_ABORT_REQUESTED ((hmThreadState)2)
#define HM_THREAD_STATE_STOPPED         ((hmThreadState)3)

typedef hmError(*hmThreadStartFunc)(void* user_data);

typedef struct _hmThread {
    void*  platform_data; /* First, platform-specific data are hidden from header files.
                             Second, an additional redirection allows to decouple a running thread's lifetime
                             from its variable's lifetime (for example, a thread is still running, but hmThreadDispose was called). */
} hmThread;

/* Creates and starts a new thread. The allocator must be thread-safe, as it will allocate/deallocate on different threads.
   `name` is the name of the thread, for debugging purposes. The string will be duplicated because we must ensure it's allocated
   using a thread-safe allocator; can be HM_NULL. */
hmError hmCreateThread(
    struct _hmAllocator* allocator,
    hmString*            name,
    hmThreadStartFunc    thread_func,
    void*                user_data,
    hmThread*            in_thread
);
/* Disposes of a thread. If the thread is still running, the thread will be automatically disposed when it finishes. */
hmError hmThreadDispose(hmThread* thread);
/* Requests the thread to be aborted gracefully (cooperatively).
   The thread should poll for hmThreadGetState() == HM_THREAD_STATE_ABORT_REQUESTED and finish execution on its own
   to respect this function. */
hmError hmThreadAbort(hmThread* thread);
/* Blocks the current thread until the specified thread finishes or the specified interval in milliseconds (timeout_ms)
   elapses. Can be used together with hmThreadAbort.
   Returns HM_ERROR_INVALID_ARGUMENT if `thread` refers to the current thread. Returns HM_ERROR_TIMEOUT if the timeout expired.
   `timeout_ms` must be in the range between HM_THREAD_JOIN_MIN_TIMEOUT_MS and HM_THREAD_JOIN_MAX_TIMEOUT_MS. */
hmError hmThreadJoin(hmThread* thread, hm_millis timeout_ms);
hmThreadState hmThreadGetState(hmThread* thread);
/* Returns the name of the thread, for debugging purposes. The value should be disposed with hmStringDispose --
   it's duplicated because a thread's lifetime is not predictable, it can get disposed while we access the name value. */
hmError hmThreadGetName(hmThread* thread, hmString* in_string);
/* Returns the total CPU time for this thread. Useful for debugging CPU load. */
hm_millis hmThreadGetProcessorTime(hmThread* thread);
/* Returns the error as returned by hmThreadStartFunc when the thread finishes. Returns HM_OK if the thread hasn't finished yet. */
hmError hmThreadGetExitError(hmThread* thread);
/* Blocks the current thread for the specified number of milliseconds. The number of milliseconds must be in the range
   between HM_SLEEP_MIN_MS and HM_SLEEP_MAX_MS, otherwise HM_ERROR_INVALID_ARGUMENT is returned. */
hmError hmSleep(hm_millis ms);

#endif /* HM_THREAD_H */
