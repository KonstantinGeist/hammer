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

#include "common.h"
#include <collections/queue.h>

static hm_nint item_dispose_sum = 0;

static void create_integer_queue_and_allocator(hm_bool is_bounded, hmQueue* queue, hmAllocator* allocator)
{
    HM_TEST_INIT_ALLOC(allocator);
    HM_TEST_TRACK_OOM(allocator, HM_FALSE);
    hmError err = hmCreateQueue(
        allocator,
        sizeof(hm_nint),
        HM_DEFAULT_QUEUE_CAPACITY,
        HM_NULL, /* item_dispose_func */
        is_bounded,
        queue
    );
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(allocator, HM_TRUE);
}

static void dispose_queue_and_allocator(hmQueue* queue, hmAllocator* allocator)
{
    hmError err = hmQueueDispose(queue);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(allocator);
}

static void test_can_create_and_dispose_empty_queue()
{
    hmQueue queue;
    hmAllocator allocator;
    create_integer_queue_and_allocator(HM_TRUE, &queue, &allocator);
    hm_bool is_empty = hmQueueIsEmpty(&queue);
    HM_TEST_ASSERT(is_empty == HM_TRUE);
    hm_nint count = hmQueueGetCount(&queue);
    HM_TEST_ASSERT(count == 0);
    dispose_queue_and_allocator(&queue, &allocator);
}

static void test_can_enqueue_and_dequeue_from_queue_within_initial_capacity()
{
    hmQueue queue;
    hmAllocator allocator;
    create_integer_queue_and_allocator(HM_TRUE, &queue, &allocator);
    for (hm_nint i = 0; i < HM_DEFAULT_QUEUE_CAPACITY; i++) {
        hm_nint value = i * 2;
        hmError err = hmQueueEnqueue(&queue, &value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        hm_nint count = hmQueueGetCount(&queue);
        HM_TEST_ASSERT(count == i + 1);
        hm_bool is_empty = hmQueueIsEmpty(&queue);
        HM_TEST_ASSERT(is_empty == HM_FALSE);
    }
    for (hm_nint i = 0; i < HM_DEFAULT_QUEUE_CAPACITY/2; i++) {
        hm_nint retrieved_value;
        hmError err = hmQueueDequeue(&queue, &retrieved_value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(retrieved_value == i * 2);
        hm_nint count = hmQueueGetCount(&queue);
        HM_TEST_ASSERT(count == HM_DEFAULT_QUEUE_CAPACITY - i - 1);
        hm_bool is_empty = hmQueueIsEmpty(&queue);
        HM_TEST_ASSERT(is_empty == HM_FALSE);
    }
    #define HM_QUEUE_ITEM_VALUE 666
    {
        hm_nint value = HM_QUEUE_ITEM_VALUE;
        hmError err = hmQueueEnqueue(&queue, &value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        hm_bool is_empty = hmQueueIsEmpty(&queue);
        HM_TEST_ASSERT(is_empty == HM_FALSE);
    }
    for (hm_nint i = HM_DEFAULT_QUEUE_CAPACITY/2; i < HM_DEFAULT_QUEUE_CAPACITY; i++) {
        hm_nint retrieved_value;
        hmError err = hmQueueDequeue(&queue, &retrieved_value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(retrieved_value == i * 2);
        hm_nint count = hmQueueGetCount(&queue);
        HM_TEST_ASSERT(count == HM_DEFAULT_QUEUE_CAPACITY - i);
        hm_bool is_empty = hmQueueIsEmpty(&queue);
        HM_TEST_ASSERT(is_empty == HM_FALSE);
    }
    {
        hm_nint retrieved_value;
        hmError err = hmQueueDequeue(&queue, &retrieved_value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(retrieved_value == HM_QUEUE_ITEM_VALUE);
        hm_bool is_empty = hmQueueIsEmpty(&queue);
        HM_TEST_ASSERT(is_empty == HM_TRUE);
    }
HM_TEST_ON_FINALIZE
    dispose_queue_and_allocator(&queue, &allocator);
}

static void test_can_enqueue_and_dequeue_from_queue_beyond_capacity()
{
    #define HM_QUEUE_COUNT_BEYOND_CAPACITY HM_DEFAULT_QUEUE_CAPACITY*4
    hmQueue queue;
    hmAllocator allocator;
    create_integer_queue_and_allocator(HM_FALSE, &queue, &allocator);
    for (hm_nint i = 0; i < HM_QUEUE_COUNT_BEYOND_CAPACITY; i++) {
        hm_nint value = i * 2;
        hmError err = hmQueueEnqueue(&queue, &value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        hm_nint count = hmQueueGetCount(&queue);
        HM_TEST_ASSERT(count == i + 1);
        hm_bool is_empty = hmQueueIsEmpty(&queue);
        HM_TEST_ASSERT(is_empty == HM_FALSE);
    }
    for (hm_nint i = 0; i < HM_QUEUE_COUNT_BEYOND_CAPACITY; i++) {
        hm_nint retrieved_value;
        hmError err = hmQueueDequeue(&queue, &retrieved_value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        HM_TEST_ASSERT(retrieved_value == i * 2);
        hm_nint count = hmQueueGetCount(&queue);
        HM_TEST_ASSERT(count == HM_QUEUE_COUNT_BEYOND_CAPACITY - i - 1);
        hm_bool is_empty = hmQueueIsEmpty(&queue);
        HM_TEST_ASSERT(is_empty == ((i < HM_QUEUE_COUNT_BEYOND_CAPACITY - 1) ? HM_FALSE : HM_TRUE));
    }
HM_TEST_ON_FINALIZE
    dispose_queue_and_allocator(&queue, &allocator);
}

static void test_returns_error_when_dequeing_from_empty_queue()
{
    hmQueue queue;
    hmAllocator allocator;
    create_integer_queue_and_allocator(HM_TRUE, &queue, &allocator);
    hm_nint retrieved_value;
    hmError err = hmQueueDequeue(&queue, &retrieved_value);
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_STATE);
    dispose_queue_and_allocator(&queue, &allocator);
}

hmError int_queue_dispose_func(void* object)
{
    hm_nint value = *((hm_nint*)(object));
    item_dispose_sum += value;
    return HM_OK;
}

static void test_queue_disposes_items_on_disposal()
{
    hmQueue queue;
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmError err = hmCreateQueue(
        &allocator,
        sizeof(hm_nint),
        HM_DEFAULT_QUEUE_CAPACITY,
        &int_queue_dispose_func,
        HM_FALSE,
        &queue
    );
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hm_nint item_dispose_sum_control = 0;
    item_dispose_sum = 0;
    for (hm_nint i = 0; i < HM_DEFAULT_QUEUE_CAPACITY; i++) {
        hm_nint value = i * 2;
        hmError err = hmQueueEnqueue(&queue, &value);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        item_dispose_sum_control += value;
    }
HM_TEST_ON_FINALIZE
    dispose_queue_and_allocator(&queue, &allocator);
    if (!HM_TEST_IS_OOM()) {
        HM_TEST_ASSERT(item_dispose_sum_control == item_dispose_sum);
    }
}

static void test_returns_limit_exceeded_when_queue_is_full()
{
    hmQueue queue;
    hmAllocator allocator;
    create_integer_queue_and_allocator(HM_TRUE, &queue, &allocator);
    for (hm_nint i = 0; i < HM_DEFAULT_QUEUE_CAPACITY+1; i++) {
        hm_nint value = i * 2;
        hmError err = hmQueueEnqueue(&queue, &value);
        if (i < HM_DEFAULT_QUEUE_CAPACITY) {
            HM_TEST_ASSERT_OK_OR_OOM(err);
        } else {
            HM_TEST_ASSERT(err == HM_ERROR_LIMIT_EXCEEDED);
        }
    }
HM_TEST_ON_FINALIZE
    dispose_queue_and_allocator(&queue, &allocator);
}

HM_TEST_SUITE_BEGIN(queues)
    HM_TEST_RUN(test_can_create_and_dispose_empty_queue)
    HM_TEST_RUN(test_can_enqueue_and_dequeue_from_queue_within_initial_capacity)
    HM_TEST_RUN(test_can_enqueue_and_dequeue_from_queue_beyond_capacity)
    HM_TEST_RUN(test_returns_error_when_dequeing_from_empty_queue)
    HM_TEST_RUN(test_queue_disposes_items_on_disposal)
    HM_TEST_RUN(test_returns_limit_exceeded_when_queue_is_full)
HM_TEST_SUITE_END()
