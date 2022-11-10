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

#ifndef HM_QUEUE_H
#define HM_QUEUE_H

#include <core/common.h>

struct _hmAllocator;

#define HM_DEFAULT_QUEUE_CAPACITY 16

typedef struct {
    struct _hmAllocator* allocator;
    char*                items;
    hmDisposeFunc        item_dispose_func;
    hm_nint              item_size;
    hm_nint              capacity;
    hm_nint              count;
    hm_nint              read_index;
    hm_nint              write_index;
} hmQueue;

/* Creates a ring buffer-based queue. Arguments and the semantics are similar to those of hmArray (see).
   item_dispose_func can be HM_NULL. */
hmError hmCreateQueue(
    struct _hmAllocator* allocator,
    hm_nint item_size,
    hm_nint initial_capacity,
    hmDisposeFunc item_dispose_func,
    hmQueue* in_queue
);
/* Enqueues an item whose value is moved inside the queue.
   item_dispose_func (if any) will be called on the value if it's still inside the queue when hmQueueDispose is called. */
hmError hmQueueEnqueue(hmQueue* queue, void* value);
/* Dequeues an item. Its value is moved out of the queue and item_dispose_func won't be called on it in hmQueueDispose. */
hmError hmQueueDequeue(hmQueue* queue, void* in_value);
/* Disposes of the queue and calls item_dispose_func (if any) on all the items still in the queue. */
hmError hmQueueDispose(hmQueue* queue);

#define hmQueueCount(queue) (queue)->count
#define hmQueueIsEmpty(queue) ((queue)->count == 0)

#endif /* HM_QUEUE_H */
