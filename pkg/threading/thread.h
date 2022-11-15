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

#ifndef HM_THREAD_H
#define HM_THREAD_H

#include <core/common.h>
#include <threading/atomic.h>

struct _hmAllocator;
struct _hmArray;
struct _hmString;
struct _hmThread;

typedef hm_atomic_nint hmThreadState;
#define HM_THREAD_STATE_UNSTARTED       ((hmThreadState)0)
#define HM_THREAD_STATE_RUNNING         ((hmThreadState)1)
#define HM_THREAD_STATE_ABORT_REQUESTED ((hmThreadState)2)
#define HM_THREAD_STATE_STOPPED         ((hmThreadState)3)

typedef hmError(*hmThreadStartFunc)(void* user_data);

typedef struct {
    struct _hmString* name;     /* The name of the thread, for debugging purposes. The string will be duplicated because
                                   we must ensure it's allocated using a thread-safe allocator. */
    hm_int32          priority; /* Thread priority from 0 to 100. -1 to use the default priority. */
    hm_int32          affinity; /* Processor ID this thread has affinity to. -1 to use affinity assigned by the OS. */
} hmThreadProperties;

typedef struct _hmThread {
    void*  platform_data; /* First, platform-specific data are hidden from header files.
                             Second, an additional redirection allows to decouple a running thread's lifetime
                             from its variable's lifetime (for example, a thread is still running, but hmThreadDispose was called). */
} hmThread;

/* Creates and starts a new thread. The allocator should be thread-safe, as it will allocate/deallocate on different threads. */
hmError hmCreateThread(
    struct _hmAllocator* allocator,
    hmThreadProperties   properties,
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
/* Blocks the current thread until the specified thread finishes. Returns HM_INVALID_ARGUMENT if `thread` refers
   to the current thread. Can be used together with hmThreadAbort. */
hmError hmThreadJoin(hmThread* thread);
hmThreadState hmThreadGetState(hmThread* thread);
/* Returns the name of the thread, for debugging purposes. The value should be disposed with hmStringDispose --
   it's duplicated because a thread's lifetime is not predictable, it can get disposed while we access the name value. */
hmError hmThreadGetName(hmThread* thread, struct _hmString* in_string);
/* Lists all threads known to the runtime. Depending on the current platform, it can be only the threads created with
   hmCreateThread, or all threads in the system. Useful for debugging/monitoring. Should be disposed with hmArrayDispose. */
hmError hmListThreads(struct _hmArray* in_array); /* hmArray<hmThread> */
hm_nint hmThreadGetProcessorTime(hmThread* thread);
hmError hmThreadGetExitError(hmThread* thread);
hmError hmSleep(hm_nint ms);

#endif /* HM_THREAD_H */
