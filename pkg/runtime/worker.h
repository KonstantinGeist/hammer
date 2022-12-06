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

struct _hmAllocator;

typedef hmError(*hmWorkerFunc)(void* work_item);

typedef struct {
    struct _hmWorkerData* data;
} hmWorker;

/* A worker allows to process work items on a separate thread.
   The allocator should be thread-safe, as it will allocate/deallocate on different threads.
   The work queue can be made bounded. If it's bounded, the queue will never grow (see also hmWorkerEnqueueItem(..)).
   `item_dispose_func` specifies how items are disposed when they're removed from the queue after being processed. The
   function should be thread-safe, because it will be accessed on different threads.
   `name` is the name of the thread, for debugging purposes. The string will be duplicated because we must ensure it's allocated
   using a thread-safe allocator; can be HM_NULL.
   `worker_func` specifies the processing function. Note that any unexpected errors will immediately stop the worker.
    So if you instead would like to log errors and continue, then such errors should be processed inside worker_func. */
hmError hmCreateWorker(
    struct _hmAllocator* allocator,
    hmString* name,
    hmWorkerFunc worker_func,
    hmDisposeFunc item_dispose_func,
    hm_bool is_queue_bounded,
    hm_nint queue_size,
    hmWorker* in_worker
);
/* Before disposing of the worker, it should be stopped and awaited with hmWorkerStop(..) and hmWorkerWait(..)
   Returns HM_ERROR_INVALID_STATE if the worker isn't fully stopped. */
hmError hmWorkerDispose(hmWorker* worker);
/* Tells the worker to stop gracefully. The worker will finish processing the current item and stop. */
hmError hmWorkerStop(hmWorker* worker);
/* Blocks the current thread until the worker completely shuts down (after being told to do so via hmWorkerStop(..)).
   It's the only safe way to gracefully terminate a worker. Usually useful when the whole runtime terminates.
   If the worker fails to respond in a certain reasonable time limit, the whole runtime can be shut down without waiting for
   it (for example, if the worker is hanging) -- in that case, the function returns HM_ERROR_TIMEOUT.
   hmWorkerStop(..) is separated from hmWorkerWait(..) so that it was possible to initiate shutdown of several workers
   in parallel without waiting one by one. */
hmError hmWorkerWait(hmWorker* worker);
/* Enqueues a new item to be processed by the worker some time in the future on its dedicated thread when it has
   the resources to do so. If the worker's queue is bounded and it's full, returns HM_ERROR_LIMIT_EXCEEDED.
   `work_item` cannot be HM_NULL; also, the value should be thread-safe, because it will be accessed on different threads.
    The value will be passed to hmWorkerFunc(..) */
hmError hmWorkerEnqueueItem(hmWorker* worker, void* work_item);
/* Returns the name of the thread, for debugging purposes. The value should be disposed with hmStringDispose --
   it's duplicated because a worker's lifetime is not predictable, it can get disposed while we access the name value. */
hmError hmWorkerGetName(hmWorker* worker, hmString* in_string);

#endif /* HM_WORKER_H */
