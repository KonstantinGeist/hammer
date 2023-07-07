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

#ifndef HM_WORKER_H
#define HM_WORKER_H

#include <core/common.h>
#include <core/string.h>

#define HM_WORKER_MAX_ITEM_SIZE 1024

typedef hmError (*hmWorkerFunc)(void* work_item);

typedef struct {
    struct hmWorkerData_* data;
} hmWorker;

/* A worker allows to process work items on a separate thread.
   The allocator should be thread-safe, as it will allocate/deallocate on different threads.
   The work queue can be made bounded. If it's bounded, the queue will never grow (see also hmWorkerEnqueueItem(..)).
   `item_size` specifies the size of a work item; returns HM_ERROR_INVALID_ARGUMENT if it's bigger than HM_WORKER_MAX_ITEM_SIZE.
   `item_dispose_func_opt` specifies how items are disposed when they're removed from the queue after being processed. The
   function should be thread-safe, because it will be accessed on different threads. Can be HM_NULL.
   `name_opt` is the name of the thread, for debugging purposes. The string will be duplicated because we must ensure it's allocated
   using a thread-safe allocator; can be HM_NULL.
   `worker_func` specifies the processing function. Note that any unexpected errors will immediately stop the worker.
    So if you instead would like to log errors and continue, then such errors should be processed inside worker_func.
   `queue_size` specifies the internal queue size. Note that if the rate of enqueueing new items is very high and the queue
    is unbounded, the worker may fail with an out-of-memory condition. */
hmError hmCreateWorker(
    hmAllocator*  allocator,
    hmString*     name_opt,
    hmWorkerFunc  worker_func,
    hm_nint       item_size,
    hmDisposeFunc item_dispose_func_opt,
    hm_bool       is_queue_bounded,
    hm_nint       queue_size,
    hmWorker*     in_worker
);
/* Before disposing of the worker, it should be stopped and awaited with hmWorkerStop(..) and hmWorkerWait(..)
   Returns HM_ERROR_INVALID_STATE if the worker isn't fully stopped. */
hmError hmWorkerDispose(hmWorker* worker);
/* Tells the worker to stop gracefully.
   If `should_drain_queue` is set to HM_TRUE, the worker makes sure all work items currently enqueued are processed, before stopping.
   Otherwise, the worker will finish processing only the current item and stop immediately. */
hmError hmWorkerStop(hmWorker* worker, hm_bool should_drain_queue);
/* Blocks the current thread until the worker completely shuts down (after being told to do so via hmWorkerStop(..)).
   It's the only safe way to gracefully terminate a worker. Usually useful when the whole runtime terminates.
   If the worker fails to respond in a time limit defined by `timeout_ms`, the function returns HM_ERROR_TIMEOUT.
   For additional constraints, see hmThreadJoin(..)
   hmWorkerStop(..) is separated from hmWorkerWait(..) so that it was possible to initiate shutdown of several workers
   in parallel without waiting one by one. */
hmError hmWorkerWait(hmWorker* worker, hm_millis timeout_ms);
/* Enqueues a new item to be processed by the worker some time in the future on its dedicated thread when it has
   the resources to do so. If the worker's queue is bounded and it's full, returns HM_ERROR_LIMIT_EXCEEDED.
   `work_item` cannot be HM_NULL; also, the value should be thread-safe, because it will be accessed on different threads.
    The value will be passed to hmWorkerFunc(..)
    The item will be disposed of with `item_dispose_func` passed to the constructor of the worker. */
hmError hmWorkerEnqueueItem(hmWorker* worker, void* in_work_item);
/* Returns the name of the thread, for debugging purposes. The value should be disposed with hmStringDispose --
   it's duplicated because a worker's lifetime is not predictable, it can get disposed while we access the name value. */
hmError hmWorkerGetName(hmWorker* worker, hmString* in_string);

#endif /* HM_WORKER_H */
