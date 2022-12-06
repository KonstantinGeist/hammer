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

#include <runtime/worker.h>

#include <core/allocator.h>
#include <collections/queue.h>
#include <threading/mutex.h>
#include <threading/thread.h>
#include <threading/waitobject.h>

#define HM_WORKER_WAIT_TIMEOUT_MS 5000

typedef struct _hmWorkerData {
    hmAllocator*  allocator;
    hmThread      thread;
    hmQueue       queue;
    hmWaitObject  wait_object;
    hmMutex       queue_mutex;
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
    in_worker->data = data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (mutex_initialized) {
            err = hmCombineErrors(err, hmMutexDispose(&data->queue_mutex));
        }
        if (wait_object_initialized) {
            err = hmCombineErrors(err, hmWaitObjectDispose(&data->wait_object));
        }
        if (queue_initialized) {
            err = hmCombineErrors(err, hmQueueDispose(&data->queue));
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
    err = hmCombineErrors(err, hmMutexDispose(&data->queue_mutex));
    err = hmCombineErrors(err, hmWaitObjectDispose(&data->wait_object));
    err = hmCombineErrors(err, hmQueueDispose(&data->queue));
    hmFree(data->allocator, data);
    return err;
}

hmError hmWorkerStop(hmWorker* worker)
{
    hmWorkerData* data = worker->data;
    hmError err = hmWaitObjectPulse(&data->wait_object); /* Wakes up the worker to make it abort quicker. */
    return hmCombineErrors(err, hmThreadAbort(&data->thread));
}

hmError hmWorkerWait(hmWorker* worker)
{
    return hmThreadJoin(&worker->data->thread, HM_WORKER_WAIT_TIMEOUT_MS);
}

hmError hmWorkerEnqueueItem(hmWorker* worker, void* work_item)
{
    if (!work_item) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmWorkerData* data = worker->data;
    hmError err = hmMutexLock(&data->queue_mutex);
    err = hmCombineErrors(err, hmQueueEnqueue(&data->queue, work_item));
    err = hmCombineErrors(err, hmMutexUnlock(&data->queue_mutex));
    if (err != HM_OK) {
        err = hmCombineErrors(err, hmWaitObjectPulse(&data->wait_object));
    }
    return err;
}

hmError hmWorkerGetName(hmWorker* worker, hmString* in_string)
{
    return hmThreadGetName(&worker->data->thread, in_string);
}

static hmError hmWorkerThreadFunc(void* user_data)
{
    hmWorkerData* data = (hmWorkerData*)user_data;
    while (hmThreadGetState(&data->thread) != HM_THREAD_STATE_ABORT_REQUESTED) {
        hmSleep(1000);
    }
    return HM_OK;
}
