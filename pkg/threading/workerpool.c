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

#include <threading/workerpool.h>

hmError hmCreateWorkerPool(
    hmAllocator*  allocator,
    hm_nint       worker_count,
    hmWorkerFunc  worker_func,
    hm_nint       item_size,
    hmDisposeFunc item_dispose_func_opt,
    hm_bool       is_queue_bounded,
    hm_nint       queue_capacity,
    hmWorkerPool* in_worker_pool
)
{
    if (worker_count == 0) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    in_worker_pool->workers = hmAlloc(allocator, sizeof(hmWorker) * worker_count);
    if (!in_worker_pool->workers) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    hm_nint worker_index = 0;
    for (worker_index = 0; worker_index < worker_count; worker_index++) {
        HM_TRY_OR_FINALIZE(err, hmCreateWorker(
            allocator,
            HM_NULL,
            worker_func,
            item_size,
            item_dispose_func_opt,
            is_queue_bounded,
            queue_capacity,
            &in_worker_pool->workers[worker_index]
        ));
    }
    in_worker_pool->allocator = allocator;
    in_worker_pool->worker_count = worker_count;
    in_worker_pool->current_index = 0;
HM_ON_FINALIZE
    if (err != HM_OK) {
        for (hm_nint j = 0; j < worker_index; j++) {
            err = hmMergeErrors(err, hmWorkerDispose(&in_worker_pool->workers[j]));
        }
        hmFree(allocator, in_worker_pool->workers);
    }
    return err;
}

hmError hmWorkerPoolDispose(hmWorkerPool* pool)
{
    hmError err = HM_OK;
    for (hm_nint i = 0; i < pool->worker_count; i++) {
        err = hmMergeErrors(err, hmWorkerDispose(&pool->workers[i]));
    }
    hmFree(pool->allocator, pool->workers);
    return err;
}

hmError hmWorkerPoolStop(hmWorkerPool* pool, hm_bool should_drain_queue)
{
    hmError err = HM_OK;
    for (hm_nint i = 0; i < pool->worker_count; i++) {
        err = hmMergeErrors(err, hmWorkerStop(&pool->workers[i], should_drain_queue));
    }
    return err;
}

hmError hmWorkerPoolWait(hmWorkerPool* pool, hm_millis timeout_ms)
{
    hmError err = HM_OK;
    for (hm_nint i = 0; i < pool->worker_count; i++) {
        err = hmMergeErrors(err, hmWorkerWait(&pool->workers[i], timeout_ms));
    }
    return err;
}

hmError hmWorkerPoolEnqueueItem(hmWorkerPool* pool, void* in_work_item)
{
    /* A combination of "round robin" and "power of two choices" load balancing algorithms: move the current index forward
       and choose the worker with the smallest queue inside the sliding window of 2. */
    hm_nint current_index = (hm_nint)hmAtomicIncrement(&pool->current_index);
    hmWorker* first_choice = &pool->workers[current_index % pool->worker_count];
    hmWorker* second_choice = &pool->workers[(current_index + 1) % pool->worker_count];
    hmWorker* worker = hmWorkerGetQueueSize(first_choice) < hmWorkerGetQueueSize(second_choice) ? first_choice : second_choice;
    return hmWorkerEnqueueItem(worker, in_work_item);
}
