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

#include <collections/queue.h>
#include <core/allocator.h>

#define HM_QUEUE_GROWTH_FACTOR 2

static hmError hmQueueDoubleQueue(hmQueue* queue);
#define hmQueueIncrementIndex(queue, index) (((index) + 1) % (queue)->capacity)

hmError hmCreateQueue(
    struct _hmAllocator* allocator,
    hm_nint item_size,
    hm_nint initial_capacity,
    hmDisposeFunc item_dispose_func,
    hm_bool is_bounded,
    hmQueue* in_queue
)
{
    if (!item_size || !initial_capacity) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    char* items = (char*)hmAlloc(allocator, item_size * initial_capacity);
    if (!items) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    in_queue->allocator = allocator;
    in_queue->items = items;
    in_queue->item_dispose_func = item_dispose_func;
    in_queue->item_size = item_size;
    in_queue->capacity = initial_capacity;
    in_queue->count = 0;
    in_queue->read_index = 0;
    in_queue->write_index = 0;
    in_queue->is_bounded = is_bounded;
    return HM_OK;
}

hmError hmQueueDispose(hmQueue* queue)
{
    hmError err = HM_OK;
    if (queue->item_dispose_func) {
        for(hm_nint i = 0; i < queue->count; i++, queue->read_index = hmQueueIncrementIndex(queue, queue->read_index)) {
            err = hmCombineErrors(err, queue->item_dispose_func(queue->items + queue->item_size * queue->read_index));
        }
    }
    hmFree(queue->allocator, queue->items);
    return err;
}

hmError hmQueueEnqueue(hmQueue* queue, void* value)
{
    if (queue->count == queue->capacity) {
        if (queue->is_bounded) {
            return HM_ERROR_LIMIT_EXCEEDED;
        }
        HM_TRY(hmQueueDoubleQueue(queue));
    }
    memcpy(queue->items + queue->item_size * queue->write_index, value, queue->item_size);
    queue->write_index = hmQueueIncrementIndex(queue, queue->write_index);
    queue->count++;
    return HM_OK;
}

hmError hmQueueDequeue(hmQueue* queue, void* in_value)
{
    if (!queue->count) {
        return HM_ERROR_INVALID_STATE;
    }
    memcpy(in_value, queue->items + queue->item_size * queue->read_index, queue->item_size);
    queue->read_index = hmQueueIncrementIndex(queue, queue->read_index);
    queue->count--;
    return HM_OK;
}

static hmError hmQueueDoubleQueue(hmQueue* queue)
{
    hm_nint new_capacity = queue->capacity * HM_QUEUE_GROWTH_FACTOR;
    char* new_items = (char*)hmAlloc(queue->allocator, queue->item_size * new_capacity);
    if (!new_items) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    for (hm_nint i = 0; i < queue->count; i++, queue->read_index = hmQueueIncrementIndex(queue, queue->read_index)) {
        memcpy(new_items + i * queue->item_size, queue->items + queue->read_index * queue->item_size, queue->item_size);
    }
    hmFree(queue->allocator, queue->items);
    queue->items = new_items;
    queue->capacity = new_capacity;
    queue->read_index = 0;
    queue->write_index = queue->count;
    return HM_OK;
}