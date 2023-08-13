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
    hm_atomic_nint current_index;
} hmWorkerPool;

/* A worker pool is a way to multiplex workers onto all available CPU's.
   `worker_count` specifies the number of workers. Usually, the value is set to be equal to the number of CPU's on the
    system (see hmGetProcessorCount())
   `worker_func` is called every time a new item needs to be processed.
   `item_dispose_func_opt` specifies how items are disposed when they're removed from the worker's queue after being
    processed. The function should be thread-safe, because it will be accessed on different threads. Can be HM_NULL.
   `is_queue_bounded` specifies whether the workers' queues are bounded or unbounded. Unbounded queues grow infinitely,
    while bounded queues return HM_ERROR_LIMIT_EXCEEDED if the capacity is exceeded. See also hmCreateQueue(..)
   `queue_capacity` specifies the internal queue size. Note that if the rate of enqueueing new items is very high and
    the queue is unbounded, the chosen worker may fail with an out-of-memory condition. */
hmError hmCreateWorkerPool(
    hmAllocator*  allocator,
    hm_nint       worker_count,
    hmWorkerFunc  worker_func,
    hm_nint       item_size,
    hmDisposeFunc item_dispose_func_opt,
    hm_bool       is_queue_bounded,
    hm_nint       queue_capacity,
    hmWorkerPool* in_worker_pool
);
hmError hmWorkerPoolDispose(hmWorkerPool* pool);
/* Tells the worker pool to stop gracefully by asking all workers in the pool to stop.
   If `should_drain_queue` is set to HM_TRUE, the workers make sure all work items currently enqueued are processed, before stopping.
   Otherwise, the workers will finish processing only the current item and stop immediately. */
hmError hmWorkerPoolStop(hmWorkerPool* pool, hm_bool should_drain_queue);
/* Waits until all workers in the pool report that they are ready to be aborted after a call to hmWorkerPoolStop(..) (see). */
hmError hmWorkerPoolWait(hmWorkerPool* pool, hm_millis timeout_ms);
/* Enqueues a new item to be processed by one of the workers some time in the future on its dedicated thread when it has
   the resources to do so. If the worker pool's internal backing queue is bounded and it's full, returns HM_ERROR_LIMIT_EXCEEDED.
  `work_item` cannot be HM_NULL; also, the value should be thread-safe, because it will be accessed on different threads.
   The value will be passed to hmWorkerFunc(..)
   The item will be disposed of with `item_dispose_func_opt` passed to the constructor of the worker. */
hmError hmWorkerPoolEnqueueItem(hmWorkerPool* pool, void* in_work_item);

#endif /* HM_WORKER_POOL_H */
