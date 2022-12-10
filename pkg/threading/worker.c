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

#include <threading/worker.h>

#include <core/allocator.h>
#include <collections/queue.h>
#include <threading/mutex.h>
#include <threading/thread.h>
#include <threading/waitobject.h>

/* A reasonable timeout just in case something is wrong with our WaitObject implementation and the whole thing hangs --
   if we want to stop the worker, it will eventually reactivate and stop in any case. */
#define HM_WORKER_THREAD_WAIT_TIMEOUT_MS 4000

typedef struct _hmWorkerData {
    hmAllocator*   allocator;
    hmThread       thread;
    hmQueue        queue;
    hmWaitObject   wait_object;
    hmMutex        queue_mutex;
    hmDisposeFunc  item_dispose_func;
    hmWorkerFunc   worker_func;
volatile
    hm_atomic_bool should_drain_queue;
    hm_bool        is_draining_queue;
} hmWorkerData;

static hmError hmWorkerThreadFunc(void* user_data);

hmError hmCreateWorker(
    struct _hmAllocator* allocator,
    hmString* name,
    hmWorkerFunc worker_func,
    hmDisposeFunc item_dispose_func,
    hm_bool is_queue_bounded,
    hm_nint queue_size,
    hmWorker* in_worker
)
{
    if (!worker_func) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmWorkerData* data = (hmWorkerData*)hmAlloc(allocator, sizeof(hmWorkerData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hm_bool queue_initialized       = HM_FALSE,
            wait_object_initialized = HM_FALSE,
            mutex_initialized       = HM_FALSE;
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmCreateQueue(
        allocator,
        sizeof(void*),
        queue_size,
        item_dispose_func,
        is_queue_bounded,
        &data->queue
    ));
    queue_initialized = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmCreateWaitObject(allocator, &data->wait_object));
    wait_object_initialized = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmCreateMutex(allocator, &data->queue_mutex));
    mutex_initialized = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmCreateThread(allocator, name, &hmWorkerThreadFunc, data, &data->thread));
    data->allocator = allocator;
    data->item_dispose_func = item_dispose_func;
    data->worker_func = worker_func;
    hmAtomicStore(&data->should_drain_queue, HM_FALSE);
    data->is_draining_queue = HM_FALSE;
    in_worker->data = data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (mutex_initialized) {
            err = hmMergeErrors(err, hmMutexDispose(&data->queue_mutex));
        }
        if (wait_object_initialized) {
            err = hmMergeErrors(err, hmWaitObjectDispose(&data->wait_object));
        }
        if (queue_initialized) {
            err = hmMergeErrors(err, hmQueueDispose(&data->queue));
        }
        hmFree(allocator, data);
    }
    return err;
}

hmError hmWorkerDispose(hmWorker* worker)
{
    hmWorkerData* data = worker->data;
    if (hmThreadGetState(&data->thread) != HM_THREAD_STATE_STOPPED) {
        return HM_ERROR_INVALID_STATE;
    }
    hmError err = hmThreadDispose(&data->thread);
    err = hmMergeErrors(err, hmMutexDispose(&data->queue_mutex));
    err = hmMergeErrors(err, hmWaitObjectDispose(&data->wait_object));
    err = hmMergeErrors(err, hmQueueDispose(&data->queue));
    hmFree(data->allocator, data);
    return err;
}

hmError hmWorkerStop(hmWorker* worker, hm_bool should_drain_queue)
{
    hmWorkerData* data = worker->data;
    hmAtomicStore(&data->should_drain_queue, should_drain_queue);
    HM_TRY(hmThreadAbort(&data->thread));
    return hmWaitObjectPulse(&data->wait_object); /* Wakes up the worker to make it abort quicker (without waiting for the timeout). */
}

hmError hmWorkerWait(hmWorker* worker, hm_millis timeout_ms)
{
    return hmThreadJoin(&worker->data->thread, timeout_ms);
}

hmError hmWorkerEnqueueItem(hmWorker* worker, void* work_item)
{
    if (!work_item) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmWorkerData* data = worker->data;
    HM_TRY(hmMutexLock(&data->queue_mutex));
    hmError err = hmQueueEnqueue(&data->queue, &work_item);
    HM_TRY(hmMergeErrors(err, hmMutexUnlock(&data->queue_mutex)));
    return hmWaitObjectPulse(&data->wait_object);
}

hmError hmWorkerGetName(hmWorker* worker, hmString* in_string)
{
    return hmThreadGetName(&worker->data->thread, in_string);
}

static hmError hmWorkerDequeueWorkItem(hmWorkerData* data, void** out_work_item)
{
    HM_TRY(hmMutexLock(&data->queue_mutex));
    hmError err = hmQueueDequeue(&data->queue, out_work_item);
    return hmMergeErrors(err, hmMutexUnlock(&data->queue_mutex));
}

static hm_bool hmWorkerShouldRun(hmWorkerData* data)
{
    return hmThreadGetState(&data->thread) != HM_THREAD_STATE_ABORT_REQUESTED;
}

static hm_bool hmWorkerShouldDrainQueue(hmWorkerData* data)
{
    return hmAtomicLoad(&data->should_drain_queue) == HM_TRUE;
}

static hm_bool hmWorkerShouldProcessQueue(hmWorkerData* data)
{
    return data->is_draining_queue || hmThreadGetState(&data->thread) != HM_THREAD_STATE_ABORT_REQUESTED;
}

static hmError hmWorkerWaitForNewItems(hmWorkerData* data)
{
    hmError err = hmWaitObjectWait(&data->wait_object, HM_WORKER_THREAD_WAIT_TIMEOUT_MS);
    return err == HM_ERROR_TIMEOUT ? HM_OK : err; /* It's OK if we time out here. */
}

static hmError hmWorkerProcessNewItems(hmWorkerData* data)
{
    hmError err = HM_OK;
    void* work_item = HM_NULL;
    while (hmWorkerShouldProcessQueue(data) && (err = hmWorkerDequeueWorkItem(data, &work_item)) == HM_OK) {
        err = data->worker_func(work_item);
        if (data->item_dispose_func) {
            err = hmMergeErrors(err, data->item_dispose_func(work_item));
        }
        HM_TRY(err);
    }
    /* HM_ERROR_INVALID_STATE means there's no more work items in the queue. */
    return err == HM_ERROR_INVALID_STATE ? HM_OK : err;
}

static hmError hmWorkerDrainQueue(hmWorkerData* data)
{
    data->is_draining_queue = HM_TRUE;
    return hmWorkerProcessNewItems(data);
}

static hmError hmWorkerThreadFunc(void* user_data)
{
    hmWorkerData* data = (hmWorkerData*)user_data;
    while (hmWorkerShouldRun(data)) {
        HM_TRY(hmWorkerWaitForNewItems(data));
        HM_TRY(hmWorkerProcessNewItems(data));
    }
    if (hmWorkerShouldDrainQueue(data)) {
        HM_TRY(hmWorkerDrainQueue(data));
    }
    return HM_OK;
}
