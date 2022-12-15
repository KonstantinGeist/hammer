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

#include <collections/queue.h>
#include <core/allocator.h>
#include <core/math.h>
#include <core/utils.h>

#define HM_QUEUE_GROWTH_FACTOR 2

static hmError hmQueueDoubleQueue(hmQueue* queue);
/* No hmAddNint because even with wrap-arounds, it will fit in the buffer due to the modulo operator. */
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
    hm_nint items_size = 0;
    HM_TRY(hmMulNint(item_size, initial_capacity, &items_size));
    char* items = (char*)hmAlloc(allocator, items_size);
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
            /* No safe math operations here because it was prevalidated before. */
            err = hmMergeErrors(err, queue->item_dispose_func(queue->items + queue->item_size * queue->read_index));
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
    hm_nint new_count = 0;
    HM_TRY(hmAddNint(queue->count, 1, &new_count));
    hm_nint item_address = 0;
    HM_TRY(hmAddMulNint((hm_nint)queue->items, queue->item_size, queue->write_index, &item_address));
    hmCopyMemory((char*)item_address, value, queue->item_size);
    queue->write_index = hmQueueIncrementIndex(queue, queue->write_index);
    queue->count = new_count;
    return HM_OK;
}

hmError hmQueueDequeue(hmQueue* queue, void* in_value)
{
    if (!queue->count) {
        return HM_ERROR_INVALID_STATE;
    }
    hm_nint item_address = 0;
    HM_TRY(hmAddMulNint((hm_nint)queue->items, queue->item_size, queue->read_index, &item_address));
    hmCopyMemory(in_value, (char*)item_address, queue->item_size);
    queue->read_index = hmQueueIncrementIndex(queue, queue->read_index);
    queue->count--;
    return HM_OK;
}

static hmError hmQueueDoubleQueue(hmQueue* queue)
{
    hm_nint new_capacity = 0;
    HM_TRY(hmMulNint(queue->capacity, HM_QUEUE_GROWTH_FACTOR, &new_capacity));
    hm_nint new_items_size = 0;
    HM_TRY(hmMulNint(queue->item_size, new_capacity, &new_items_size));
    char* new_items = (char*)hmAlloc(queue->allocator, new_items_size);
    if (!new_items) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    for (hm_nint i = 0; i < queue->count; i++, queue->read_index = hmQueueIncrementIndex(queue, queue->read_index)) {
        hm_nint old_items_address = 0, new_items_address = 0;
        hmError err = hmAddMulNint((hm_nint)queue->items, queue->read_index, queue->item_size, &old_items_address);
        err = hmMergeErrors(err, hmAddMulNint((hm_nint)new_items, i, queue->item_size, &new_items_address));
        if (err != HM_OK) {
            hmFree(queue->allocator, new_items);
            return err;
        }
        hmCopyMemory((char*)new_items_address, (const char*)old_items_address, queue->item_size);
    }
    hmFree(queue->allocator, queue->items);
    queue->items = new_items;
    queue->capacity = new_capacity;
    queue->read_index = 0;
    queue->write_index = queue->count;
    return HM_OK;
}
