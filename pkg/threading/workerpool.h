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

#ifndef HM_WORKER_POOL_H
#define HM_WORKER_POOL_H

#include <threading/atomic.h>
#include <threading/worker.h>

typedef struct {
    hmAllocator*   allocator;
    hmWorker*      workers;
    hm_nint        worker_count;
    hm_atomic_nint cur_index;
} hmWorkerPool;

hmError hmCreateWorkerPool(
    hmAllocator*  allocator,
    hm_nint       worker_count,
    hmWorkerFunc  worker_func,
    hm_nint       item_size,
    hmDisposeFunc item_dispose_func_opt,
    hm_bool       is_queue_bounded,
    hm_nint       queue_size,
    hmWorkerPool* in_worker_pool
);
hmError hmWorkerPoolDispose(hmWorkerPool* pool);
hmError hmWorkerPoolStop(hmWorkerPool* pool, hm_bool should_drain_queue);
hmError hmWorkerPoolWait(hmWorkerPool* pool, hm_millis timeout_ms);
hmError hmWorkerPoolEnqueueItem(hmWorkerPool* pool, void* in_work_item);

#endif /* HM_WORKER_POOL_H */
