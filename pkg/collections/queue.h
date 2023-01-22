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

#ifndef HM_QUEUE_H
#define HM_QUEUE_H

#include <core/common.h>
#include <core/allocator.h>

#define HM_QUEUE_DEFAULT_CAPACITY 16

typedef struct {
    hmAllocator*  allocator;
    char*         items;
    hmDisposeFunc item_dispose_func;
    hm_nint       item_size;
    hm_nint       capacity;
    hm_nint       count;
    hm_nint       read_index;
    hm_nint       write_index;
    hm_bool       is_bounded;
} hmQueue;

/* Creates a ring buffer-based queue. Arguments and the semantics are similar to those of hmArray (see).
   item_dispose_func can be HM_NULL. */
hmError hmCreateQueue(
    hmAllocator*  allocator,
    hm_nint       item_size,
    hm_nint       initial_capacity,
    hmDisposeFunc item_dispose_func,
    hm_bool       is_bounded, /* A bounded queue returns HM_ERROR_LIMIT_EXCEEDED if the queue is full, instead of increasing the capacity. */
    hmQueue*      in_queue
);
/* Disposes of the queue and calls item_dispose_func (if any) on all the items still in the queue. */
hmError hmQueueDispose(hmQueue* queue);
/* Enqueues an item whose value is moved inside the queue.
   item_dispose_func (if any) will be called on the value if it's still inside the queue when hmQueueDispose is called.
   If the queue is bounded and it's full, returns HM_ERROR_LIMIT_EXCEEDED.
   If the queue is unbounded and it's full, doubles the capacity. */
hmError hmQueueEnqueue(hmQueue* queue, void* value);
/* Dequeues an item. Its value is moved out of the queue and item_dispose_func won't be called on it in hmQueueDispose.
   Returns HM_ERROR_INVALID_STATE if there are no more items in the queue. */
hmError hmQueueDequeue(hmQueue* queue, void* in_value);

#define hmQueueGetCount(queue) (queue)->count
#define hmQueueIsEmpty(queue) ((queue)->count == 0)

#endif /* HM_QUEUE_H */
