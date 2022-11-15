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

#include <threading/thread.h>
#include <core/allocator.h>
#include <core/string.h>
#include <collections/array.h>

#include <pthread.h>
#include <time.h>

#define POSIX_RESULT_OK 0

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
    hmError           exit_error;
} hmThreadPlatformData;

#define hmResultToError(result) ((result) != POSIX_RESULT_OK ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK)
#define hmThreadGetPlatformData(thread) ((hmThreadPlatformData*)(thread)->platform_data)
static hmError hmThreadDisposePlatformData(hmThreadPlatformData* platform_data);
static void* hmAdaptPosixThreadToHammer(void* arg);

hmError hmCreateThread(
    struct _hmAllocator* allocator,
    hmThreadProperties   properties,
    hmThreadStartFunc    thread_func,
    void*                user_data,
    hmThread*            in_thread
)
{
    hmThreadPlatformData* platform_data = (hmThreadPlatformData*)hmAlloc(allocator, sizeof(hmThreadPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    hm_bool is_string_duplicated = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmStringDuplicate(allocator, (hmString*)properties.name, &platform_data->name));
    is_string_duplicated = HM_TRUE;
    hmAtomicStore(platform_data->ref_count, 2); /* +1 reference for the object itself, +1 reference to auto-dispose when the thread finishes */
    platform_data->allocator = allocator;
    platform_data->user_data = user_data;
    platform_data->thread_func = thread_func;
    hmAtomicStore(platform_data->state, HM_THREAD_STATE_UNSTARTED);
    platform_data->exit_error = HM_OK;
    in_thread->platform_data = platform_data;
    HM_TRY_OR_FINALIZE(err, hmResultToError(pthread_create(&platform_data->posix_thread, 0, &hmAdaptPosixThreadToHammer, platform_data)));
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (is_string_duplicated) {
            err = hmCombineErrors(err, hmStringDispose(&platform_data->name));
        }
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmThreadDispose(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    return hmThreadDisposePlatformData(platform_data);
}

hmError hmThreadAbort(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    /* It's a bit racy, but quite tolerable: it's possible that a thread will end up in ABORT_REQUESTED state
       despite actually being already STOPPED if it's being stopped and its abort is requested at the same time. */
    if (hmAtomicLoad(platform_data->state) != HM_THREAD_STATE_STOPPED) {
        hmAtomicStore(platform_data->state, HM_THREAD_STATE_ABORT_REQUESTED);
    }
    return HM_OK;
}

// TODO add timeout support
// TODO check can't join itself
hmError hmThreadJoin(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    hmThreadState state = hmAtomicLoad(platform_data->state);
    if(state == HM_THREAD_STATE_STOPPED) {
        return HM_OK;
    }
    return hmResultToError(pthread_join(platform_data->posix_thread, 0));
}

hmThreadState hmThreadGetState(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    return hmAtomicLoad(platform_data->state);
}

hmError hmThreadGetName(hmThread* thread, struct _hmString* in_string)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    return hmStringDuplicate(platform_data->allocator, &platform_data->name, (hmString*)in_string);
}

hmError hmListThreads(struct _hmArray* in_array)
{
    return HM_ERROR_NOT_IMPLEMENTED;
}

hm_nint hmThreadGetProcessorTime(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    struct timespec ts;
    clockid_t cid;
    if (pthread_getcpuclockid(platform_data->posix_thread, &cid)) {
        return 0;
    }
    if (clock_gettime(cid, &ts) == -1) {
        return 0;
    }
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

hmError hmThreadGetExitError(hmThread* thread)
{
    hmThreadPlatformData* platform_data = hmThreadGetPlatformData(thread);
    return platform_data->state;
}

hmError hmSleep(hm_nint ms)
{
    struct timespec ts;
    ts.tv_sec = (uint64_t)ms / 1000;              /* Seconds. */
    ts.tv_nsec = ((uint64_t)ms % 1000) * 1000000; /* Milliseconds. */
    nanosleep(&ts, NULL);
    return HM_OK;
}

static hmError hmThreadDisposePlatformData(hmThreadPlatformData* platform_data)
{
    hm_nint older_ref_count = hmAtomicDecrement(platform_data->ref_count);
    hmError err = HM_OK;
    if (older_ref_count == 1) {
        err = hmCombineErrors(err, hmStringDispose(&platform_data->name));
        err = hmCombineErrors(err, hmResultToError(pthread_detach(platform_data->posix_thread)));
        hmFree(platform_data->allocator, platform_data);
    }
    return err;
}

static void* hmAdaptPosixThreadToHammer(void* arg)
{
    hmThreadPlatformData* platform_data = (hmThreadPlatformData*)arg;
    hmAtomicStore(platform_data->state, HM_THREAD_STATE_RUNNING);
    platform_data->exit_error = platform_data->thread_func(platform_data->user_data);
    hmAtomicStore(platform_data->state, HM_THREAD_STATE_STOPPED);
    // TODO where to the result of this function?
    hmThreadDisposePlatformData(platform_data); // auto-disposed when the thread finishes
    return 0;
}
