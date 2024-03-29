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

#include <threading/thread.h>
#include <core/allocator.h>
#include <core/string.h>
#include <core/utils.h>
#include <collections/array.h>
#include <platform/unix/common.h>

#include <errno.h>   /* for ETIMEDOUT */
#include <pthread.h> /* for all the POSIX thread functions */

typedef struct {
    hmAllocator*      allocator;
    void*             user_data;
    hmThreadStartFunc thread_func;
    hmString          name;
    pthread_t         posix_thread;
volatile
    hmThreadState     state;
volatile
    hm_atomic_nint    ref_count;
volatile
    hm_atomic_nint    exit_err; /* actually, hmError */
volatile
    hm_atomic_bool    is_abort_requested; /* see hmThreadGetState(..) */
volatile
    hm_atomic_bool    is_detached;
} hmThreadPlatformData;

#define hmThreadGetPlatformData(thread) ((hmThreadPlatformData*)(thread)->platform_data)
static hmError hmThreadTryDisposePlatformData(hmThreadPlatformData* platform_data);
static void* hmAdaptPosixThreadToHammer(void* arg);

hmError hmCreateThread(
    hmAllocator*      allocator,
    hmString*         name_opt,
    hmThreadStartFunc thread_func,
    void*             user_data,
    hmThread*         in_thread
)
{
    hmThreadPlatformData* platform_data = (hmThreadPlatformData*)hmAlloc(allocator, sizeof(hmThreadPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    hm_bool is_string_duplicated = HM_FALSE;
    if (name_opt) {
        HM_TRY_OR_FINALIZE(err, hmStringDuplicate(allocator, name_opt, &platform_data->name));
    } else {
        HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString("", &platform_data->name));
    }
    is_string_duplicated = HM_TRUE;
    hmAtomicStore(&platform_data->ref_count, 2); /* +1 reference for the object itself, +1 reference to auto-dispose when the thread finishes. */
    platform_data->allocator = allocator;
    platform_data->user_data = user_data;
    platform_data->thread_func = thread_func;
    hmAtomicStore(&platform_data->state, HM_THREAD_STATE_UNSTARTED);
    hmAtomicStore(&platform_data->exit_err, HM_OK);
    hmAtomicStore(&platform_data->is_abort_requested, HM_FALSE);
    hmAtomicStore(&platform_data->is_detached, HM_FALSE);
    in_thread->platform_data = platform_data;
    HM_TRY_OR_FINALIZE(err, hmUnixErrorToHammer(pthread_create(&platform_data->posix_thread, 0, &hmAdaptPosixThreadToHammer, platform_data)));
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (is_string_duplicated) {
            err = hmMergeErrors(err, hmStringDispose(&platform_data->name));
        }
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmThreadDispose(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    return hmThreadTryDisposePlatformData(platform_data);
}

hmError hmThreadAbort(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    hmAtomicStore(&platform_data->is_abort_requested, HM_TRUE); /* See hmThreadGetState(..) */
    return HM_OK;
}

hmError hmThreadJoin(hmThread* thread, hm_millis timeout_ms)
{
    if (timeout_ms < HM_THREAD_JOIN_MIN_TIMEOUT_MS || timeout_ms > HM_THREAD_JOIN_MAX_TIMEOUT_MS) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    if (pthread_equal(platform_data->posix_thread, pthread_self())) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    if (hmAtomicLoad(&platform_data->state) == HM_THREAD_STATE_STOPPED) {
        return HM_OK;
    }
    struct timespec ts;
    HM_TRY(hmGetFutureTimeSpec(HM_FALSE, timeout_ms, &ts));
    int unix_err = pthread_timedjoin_np(platform_data->posix_thread, HM_NULL, &ts);
    hmError err = hmUnixErrorToHammer(unix_err);
    if (err == HM_OK) {
        /* Makes sure we don't call pthread_detach in the destructor later on, as the thread is already
           detached after a call to pthread_join. */
        hmAtomicStore(&platform_data->is_detached, HM_TRUE);
    }
    return err;
}

hmThreadState hmThreadGetState(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    hmThreadState state = hmAtomicLoad(&platform_data->state);
    hm_atomic_bool is_abort_requested = hmAtomicLoad(&platform_data->is_abort_requested);
    /* It would be racy to immediately set `state` to HM_THREAD_STATE_ABORT_REQUESTED in hmThreadAbort(..) because in
       `thread_func` the thread's state can be changed to other values in parallel. A possible outcome: the thread
       actually stopped, but its state is reported as "abort requested" (because the thread was aborted after it stopped).
       As a solution, we split the state and the abort request into separate variables which are later merged at runtime
       when the state is requested in hmThreadGetState(..) */
    if (is_abort_requested && state != HM_THREAD_STATE_STOPPED) {
        return HM_THREAD_STATE_ABORT_REQUESTED;
    }
    return state;
}

hmError hmThreadGetName(hmThread* thread, hmString* in_string)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    return hmStringDuplicate(platform_data->allocator, &platform_data->name, (hmString*)in_string);
}

hm_millis hmThreadGetProcessorTime(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    struct timespec ts;
    clockid_t cid = 0;
    if (pthread_getcpuclockid(platform_data->posix_thread, &cid)) {
        return 0;
    }
    if (clock_gettime(cid, &ts) == -1) {
        return 0;
    }
    return hmConvertTimeSpecToMilliseconds(&ts);
}

hmError hmThreadGetExitError(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    return hmAtomicLoad(&platform_data->exit_err);
}

hmError hmSleep(hm_millis ms)
{
    if (ms < HM_SLEEP_MIN_MS || ms > HM_SLEEP_MAX_MS) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    struct timespec ts = hmConvertMillisecondsToTimeSpec(ms);
    nanosleep(&ts, NULL);
    return HM_OK;
}

static hmError hmThreadTryDisposePlatformData(hmThreadPlatformData* platform_data)
{
    hm_nint new_ref_count = hmAtomicDecrement(&platform_data->ref_count);
    hmError err = HM_OK;
    if (new_ref_count == 0) {
        err = hmMergeErrors(err, hmStringDispose(&platform_data->name));
        if (!hmAtomicLoad(&platform_data->is_detached)) {
            err = hmMergeErrors(err, hmUnixErrorToHammer(pthread_detach(platform_data->posix_thread)));
        }
        hmFree(platform_data->allocator, platform_data);
    }
    return err;
}

static void* hmAdaptPosixThreadToHammer(void* arg)
{
    hmThreadPlatformData* platform_data = (hmThreadPlatformData*)arg;
    hmAtomicStore(&platform_data->state, HM_THREAD_STATE_RUNNING);
    hmAtomicStore(&platform_data->exit_err, platform_data->thread_func(platform_data->user_data));
    hmAtomicStore(&platform_data->state, HM_THREAD_STATE_STOPPED);
    /* Auto-disposes when the thread finishes, but the thread object may still be alive because the reference count
       will definitely be 0 only with a call to hmThreadDispose(..) */
    hmError err = hmThreadTryDisposePlatformData(platform_data);
    if (err != HM_OK) {
        hmLog("hmThreadTryDisposePlatformData(..) failed"); /* Nowhere else to report it yet. */
    }
    return 0;
}
