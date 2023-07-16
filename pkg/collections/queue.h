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
    char*         items;                   /* The internal backing array which stores the items. */
    hmDisposeFunc item_dispose_func_opt;   /* Dispose function: called on every item on container destruction. Can be HM_NULL. */
    hm_nint       item_size;               /* Size of a single item (this is how we achieve genericity). */
    hm_nint       capacity;                /* The capacity of the array: determines how many items can be added before
                                              the internal backing array is resized. */
    hm_nint       count;                   /* Keeps track of how many items the queue holds. */
    hm_nint       read_index;              /* The index which specifies where reading inside `items` starts, as part of
                                              the classic ring buffer implementation. */
    hm_nint       write_index;             /* The index which specifies where writing inside `items` starts, as part of
                                              the classic ring buffer implementation. */
    hm_bool       is_bounded;              /* Specifies whether the queue is bounded or unbounded. Unbounded
                                              queues grow infinitely, while bounded queues return HM_ERROR_LIMIT_EXCEEDED
                                              if the capacity is exceeded. */
} hmQueue;

/* Creates a ring buffer-based queue. Arguments and the semantics are similar to those of hmArray (see).
   `item_size` is size of a single item (this is how we achieve genericity).
   `initial_capacity` specifies the initial size of the internal backing array; set it to a large value if the queue
    will be populated with many items. The value can be set to HM_QUEUE_DEFAULT_CAPACITY.
    Returns HM_ERROR_INVALID_ARGUMENT if it's zero.
   `item_dispose_func_opt` is an optional (can be HM_NULL) dispose function: called on every item on container destruction.
    If it's specified, it is assumed that the queue takes ownership of the item (the item is "moved" into the array).
   `is_bounded` specifies whether the queue is bounded or unbounded. Unbounded queues grow infinitely, while bounded
    queues return HM_ERROR_LIMIT_EXCEEDED if the capacity is exceeded. */
hmError hmCreateQueue(
    hmAllocator*  allocator,
    hm_nint       item_size,
    hm_nint       initial_capacity,
    hmDisposeFunc item_dispose_func_opt,
    hm_bool       is_bounded,
    hmQueue*      in_queue
);
/* Disposes of the queue and calls `item_dispose_func_opt` (if any) on all the items still in the queue. */
hmError hmQueueDispose(hmQueue* queue);
/* Enqueues an item whose value is moved inside the queue.
   `item_dispose_func` (if any) will be called on the value if it's still inside the queue when hmQueueDispose(..) is called.
   If the queue is bounded and it's full (the number of items is equal to `initial_capacity`, returns HM_ERROR_LIMIT_EXCEEDED.
   If the queue is unbounded and it's full, doubles the capacity. */
hmError hmQueueEnqueue(hmQueue* queue, void* value);
/* Dequeues an item. Its value is moved out of the queue and `item_dispose_func_opt` won't be called on it in hmQueueDispose(..)
   because the array stops owning it at that point. Returns HM_ERROR_INVALID_STATE if there are no more items in the queue. */
hmError hmQueueDequeue(hmQueue* queue, void* in_value);
/* Gets the number of items actually contained in the queue. Implemented as a macro so must be OK to use as part of a
   condition in a loop. */
#define hmQueueGetCount(queue) (queue)->count
/* Returns true if the queue is empty. Useful when polling the queue for new items. Can be used without special thread
   synchronization. */
#define hmQueueIsEmpty(queue) ((queue)->count == 0)

#endif /* HM_QUEUE_H */
