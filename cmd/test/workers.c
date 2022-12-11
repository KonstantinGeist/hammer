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
#include <threading/worker.h>
#include <threading/thread.h>

/* These tests rely on some timing, so sporadically they can fail on busy machines. */

#define WORKER_NAME "TestWorker"
#define DEFAULT_WORKER_QUEUE_SIZE 16
#define WORKER_WAIT_TIMEOUT 4000

static hm_nint processed_count = 0;

static void create_worker_and_allocator(
    hmWorker* worker,
    hmAllocator* allocator,
    hmWorkerFunc worker_func,
    hm_nint item_size,
    hmDisposeFunc item_dispose_func,
    hm_bool is_queue_bounded,
    hm_nint queue_size
)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    hmString worker_name;
    err = hmCreateStringViewFromCString(WORKER_NAME, &worker_name);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateWorker(allocator, &worker_name, worker_func, item_size, item_dispose_func, is_queue_bounded, queue_size, worker);
    HM_TEST_ASSERT_OK(err);
}

static void create_worker_and_allocator_simple(hmWorker* worker, hmAllocator* allocator, hmWorkerFunc worker_func, hm_nint item_size)
{
    create_worker_and_allocator(worker, allocator, worker_func, item_size, HM_NULL, HM_FALSE, DEFAULT_WORKER_QUEUE_SIZE);
}

static void dispose_worker_and_allocator(hmWorker* worker, hmAllocator* allocator)
{
    hmError err = hmWorkerDispose(worker);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static hmError can_start_stop_wait_worker_and_get_name_worker_func(void* work_item)
{
    return HM_OK;
}

static void test_can_start_stop_wait_worker_and_get_name()
{
    hmWorker worker;
    hmAllocator allocator;
    create_worker_and_allocator_simple(&worker, &allocator, &can_start_stop_wait_worker_and_get_name_worker_func, sizeof(hm_nint));
    hmError err = hmWorkerStop(&worker, HM_FALSE);
    HM_TEST_ASSERT_OK(err);
    err = hmWorkerWait(&worker, WORKER_WAIT_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    hmString worker_name;
    err = hmWorkerGetName(&worker, &worker_name);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(&worker_name, WORKER_NAME));
    err = hmStringDispose(&worker_name);
    HM_TEST_ASSERT_OK(err);
    dispose_worker_and_allocator(&worker, &allocator);
}

typedef struct {
    hmAllocator* allocator;
    hm_nint      value;
} integer_work_item;

static hmError integer_work_item_dispose_func(void* obj)
{
    integer_work_item* work_item = *((integer_work_item**)obj);
    hmFree(work_item->allocator, work_item);
    return HM_OK;
}

static hmError can_process_work_items_fast_with_dispose_func_worker_func(void* obj)
{
    integer_work_item* work_item = *((integer_work_item**)obj);
    processed_count += work_item->value;
    return HM_OK;
}

static void test_can_process_work_items_fast_with_dispose_func()
{
    hmWorker worker;
    hmAllocator allocator;
    create_worker_and_allocator(
        &worker,
        &allocator,
        &can_process_work_items_fast_with_dispose_func_worker_func,
        sizeof(integer_work_item*),
        &integer_work_item_dispose_func,
        HM_FALSE,
        DEFAULT_WORKER_QUEUE_SIZE
    );
    processed_count = 0;
    for (hm_nint i = 0; i <= 1000; i++) {
        integer_work_item* arg = hmAlloc(&allocator, sizeof(integer_work_item));
        HM_TEST_ASSERT(arg);
        arg->allocator = &allocator;
        arg->value = i;
        hmError err = hmWorkerEnqueueItem(&worker, &arg);
        HM_TEST_ASSERT_OK(err);
    }
    hmError err = hmWorkerStop(&worker, HM_TRUE);
    HM_TEST_ASSERT_OK(err);
    err = hmWorkerWait(&worker, WORKER_WAIT_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(processed_count == 500500);
    dispose_worker_and_allocator(&worker, &allocator);
}

static hmError worker_drains_queue_when_stopped_worker_func(void* obj)
{
    integer_work_item* work_item = *((integer_work_item**)obj);
    processed_count += work_item->value;
    hmSleep(200);
    return HM_OK;
}

static void test_worker_drains_queue_when_stopped()
{
    hmWorker worker;
    hmAllocator allocator;
    create_worker_and_allocator(
        &worker,
        &allocator,
        &worker_drains_queue_when_stopped_worker_func,
        sizeof(integer_work_item*),
        &integer_work_item_dispose_func,
        HM_FALSE,
        DEFAULT_WORKER_QUEUE_SIZE
    );
    processed_count = 0;
    for (hm_nint i = 0; i <= 3; i++) {
        integer_work_item* arg = hmAlloc(&allocator, sizeof(integer_work_item));
        HM_TEST_ASSERT(arg);
        arg->allocator = &allocator;
        arg->value = i;
        hmError err = hmWorkerEnqueueItem(&worker, &arg);
        HM_TEST_ASSERT_OK(err);
    }
    hmError err = hmWorkerStop(&worker, HM_TRUE);
    HM_TEST_ASSERT_OK(err);
    err = hmWorkerWait(&worker, WORKER_WAIT_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(processed_count == 6);
    dispose_worker_and_allocator(&worker, &allocator);
}

static hmError worker_does_not_drain_queue_when_stopped_worker_func(void* obj)
{
    integer_work_item* work_item = *((integer_work_item**)obj);
    processed_count += work_item->value;
    hmSleep(200);
    return HM_OK;
}

static void test_worker_does_not_drain_queue_when_stopped()
{
    hmWorker worker;
    hmAllocator allocator;
    create_worker_and_allocator(
        &worker,
        &allocator,
        &worker_does_not_drain_queue_when_stopped_worker_func,
        sizeof(integer_work_item*),
        &integer_work_item_dispose_func,
        HM_FALSE,
        DEFAULT_WORKER_QUEUE_SIZE
    );
    processed_count = 0;
    for (hm_nint i = 0; i <= 3; i++) {
        integer_work_item* arg = hmAlloc(&allocator, sizeof(integer_work_item));
        HM_TEST_ASSERT(arg);
        arg->allocator = &allocator;
        arg->value = i;
        hmError err = hmWorkerEnqueueItem(&worker, &arg);
        HM_TEST_ASSERT_OK(err);
    }
    hmError err = hmWorkerStop(&worker, HM_FALSE);
    HM_TEST_ASSERT_OK(err);
    err = hmWorkerWait(&worker, WORKER_WAIT_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(processed_count != 6);
    dispose_worker_and_allocator(&worker, &allocator);
}

static hmError worker_returns_error_if_item_size_is_too_big_thread_func(void* obj)
{
    return HM_OK;
}

void test_worker_returns_error_if_item_size_is_too_big()
{
    hmWorker worker;
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateWorker(
        &allocator,
        HM_NULL,
        &worker_returns_error_if_item_size_is_too_big_thread_func,
        HM_WORKER_MAX_ITEM_SIZE + 1,
        HM_NULL,
        HM_TRUE,
        DEFAULT_WORKER_QUEUE_SIZE,
        &worker);
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_ARGUMENT);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

static hmError worker_can_enqueue_by_value_worker_func(void* obj)
{
    integer_work_item* work_item = (integer_work_item*)obj;
    processed_count += work_item->value;
    return HM_OK;
}

static void test_worker_can_enqueue_by_value()
{
    hmWorker worker;
    hmAllocator allocator;
    create_worker_and_allocator(
        &worker,
        &allocator,
        &worker_can_enqueue_by_value_worker_func,
        sizeof(integer_work_item),
        HM_NULL,
        HM_FALSE,
        DEFAULT_WORKER_QUEUE_SIZE
    );
    processed_count = 0;
    for (hm_nint i = 0; i <= 1000; i++) {
        integer_work_item arg;
        arg.allocator = HM_NULL;
        arg.value = i;
        hmError err = hmWorkerEnqueueItem(&worker, &arg);
        HM_TEST_ASSERT_OK(err);
    }
    hmError err = hmWorkerStop(&worker, HM_TRUE);
    HM_TEST_ASSERT_OK(err);
    err = hmWorkerWait(&worker, WORKER_WAIT_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(processed_count == 500500);
    dispose_worker_and_allocator(&worker, &allocator);
}

void test_workers()
{
    HM_TEST_SUITE_BEGIN("Workers");
        HM_TEST_RUN_WITHOUT_OOM(test_can_start_stop_wait_worker_and_get_name);
        HM_TEST_RUN_WITHOUT_OOM(test_can_process_work_items_fast_with_dispose_func);
        HM_TEST_RUN_WITHOUT_OOM(test_worker_drains_queue_when_stopped);
        HM_TEST_RUN_WITHOUT_OOM(test_worker_does_not_drain_queue_when_stopped);
        HM_TEST_RUN_WITHOUT_OOM(test_worker_returns_error_if_item_size_is_too_big);
        HM_TEST_RUN_WITHOUT_OOM(test_worker_can_enqueue_by_value);
    HM_TEST_SUITE_END();
}
