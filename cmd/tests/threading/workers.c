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

#include "../common.h"
#include <core/environment.h>
#include <threading/worker.h>
#include <threading/thread.h>

/* These tests rely on some timing, so sporadically they can fail on busy machines. */

#define WORKER_NAME "TestWorker"
#define DEFAULT_WORKER_QUEUE_SIZE 16
#define WORKER_WAIT_TIMEOUT 4000
#define THROUGHPUT_WORK_ITEM_COUNT 1000000

static hm_atomic_nint processed_count = 0;

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
    return hmSleep(200);
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
    return hmSleep(200);
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

typedef struct {
    hm_millis start_time;
    hm_millis end_time;
} throughput_work_item;

static hmError worker_throughput_worker_with_tick_count_func(void* obj)
{
    throughput_work_item* work_item = *((throughput_work_item**)obj);
    work_item->end_time = hmGetTickCount();
    (void)hmAtomicIncrement(&processed_count);
    return HM_OK;
}

static hmError worker_throughput_worker_without_tick_count_func(void* obj)
{
    return HM_OK;
}

static void worker_throughput_calculate_times(hm_bool with_tick_count, hm_float64* out_average_latency, hm_float64* out_total_time, hm_float64* out_enqueue_time)
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hm_nint worker_count = hmGetProcessorCount();
    hmWorker* workers = (hmWorker*)hmAlloc(&allocator, sizeof(hmWorker) * worker_count);
    HM_TEST_ASSERT(workers != HM_NULL);
    hmString worker_name;
    err = hmCreateStringViewFromCString(WORKER_NAME, &worker_name);
    HM_TEST_ASSERT_OK(err);
    for (hm_nint i = 0; i < worker_count; i++) {
        err = hmCreateWorker(
            &allocator,
            &worker_name,
            with_tick_count ? &worker_throughput_worker_with_tick_count_func : &worker_throughput_worker_without_tick_count_func,
            sizeof(throughput_work_item),
            HM_NULL,
            HM_FALSE,
            DEFAULT_WORKER_QUEUE_SIZE,
            &workers[i]
        );
        HM_TEST_ASSERT_OK(err);
    }
    throughput_work_item* work_items = (throughput_work_item*)hmAlloc(&allocator, sizeof(throughput_work_item) * THROUGHPUT_WORK_ITEM_COUNT);
    HM_TEST_ASSERT(work_items != HM_NULL);
    hm_millis total_start_time = hmGetTickCount();
    processed_count = 0;
    for (hm_nint i = 0; i < THROUGHPUT_WORK_ITEM_COUNT; i++) {
        throughput_work_item* arg = &work_items[i];
        if (with_tick_count) {
            arg->start_time = hmGetTickCount();
        }
        hm_nint worker_index = i % worker_count;
        err = hmWorkerEnqueueItem(&workers[worker_index], &arg);
        HM_TEST_ASSERT_OK(err);
    }
    if (out_enqueue_time) {
        *out_enqueue_time = hmGetTickCount() - total_start_time;
    }
    for (hm_nint i = 0; i < worker_count; i++) {
        err = hmWorkerStop(&workers[i], HM_TRUE);
        HM_TEST_ASSERT_OK(err);
    }
    for (hm_nint i = 0; i < worker_count; i++) {
        err = hmWorkerWait(&workers[i], WORKER_WAIT_TIMEOUT);
        HM_TEST_ASSERT_OK(err);
    }
    if (with_tick_count) {
        HM_TEST_ASSERT(processed_count == THROUGHPUT_WORK_ITEM_COUNT);
    }
    hm_float64 average_latency = 0.0;
    if (with_tick_count) {
        for (hm_nint i = 0; i < THROUGHPUT_WORK_ITEM_COUNT; i++) {
            throughput_work_item* arg = &work_items[i];
            average_latency = (average_latency + (hm_float64)(arg->end_time - arg->start_time)) / 2.0;
        }
    }
    for (hm_nint i = 0; i < worker_count; i++) {
        err = hmWorkerDispose(&workers[i]);
        HM_TEST_ASSERT_OK(err);
    }
    hmFree(&allocator, workers);
    hmFree(&allocator, work_items);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    if (out_average_latency) {
        *out_average_latency = average_latency;
    }
    if (out_total_time) {
        *out_total_time = hmGetTickCount() - total_start_time;
    }
}

// Tests enqueueing THROUGHPUT_WORK_ITEM_COUNT items + works as a benchmark (raw response time in milliseconds,
// i.e. we measure our system's overhead). Uses all available CPU cores and also additionally subtracts the time
// it takes to make hmGetTickCount() calls.
static void test_worker_throughput()
{
    hm_float64 total_time_without_tick_count, average_latency_with_tick_count, total_time_with_tick_count, enqueue_time;
    worker_throughput_calculate_times(HM_FALSE, HM_NULL, &total_time_without_tick_count, HM_NULL); // with_tick_count = HM_FALSE
    worker_throughput_calculate_times(HM_TRUE, &average_latency_with_tick_count, &total_time_with_tick_count, &enqueue_time); // with_tick_count = HM_TRUE
    hm_float64 tick_count_ratio = total_time_with_tick_count / total_time_without_tick_count;
    hm_float64 corrected_average_latency = average_latency_with_tick_count / tick_count_ratio;
    hm_float64 enqueue_rate = ((hm_float64)THROUGHPUT_WORK_ITEM_COUNT / enqueue_time) * 1000.0;
    printf("        Average latency: %.2f ms for enqueue rate = %.2f items/sec (total: %d items)\n", corrected_average_latency, enqueue_rate, THROUGHPUT_WORK_ITEM_COUNT);
}

HM_TEST_SUITE_BEGIN(workers)
    HM_TEST_RUN_WITHOUT_OOM(test_can_start_stop_wait_worker_and_get_name)
    HM_TEST_RUN_WITHOUT_OOM(test_can_process_work_items_fast_with_dispose_func)
    HM_TEST_RUN_WITHOUT_OOM(test_worker_drains_queue_when_stopped)
    HM_TEST_RUN_WITHOUT_OOM(test_worker_does_not_drain_queue_when_stopped)
    HM_TEST_RUN_WITHOUT_OOM(test_worker_returns_error_if_item_size_is_too_big)
    HM_TEST_RUN_WITHOUT_OOM(test_worker_can_enqueue_by_value)
    HM_TEST_RUN_WITHOUT_OOM(test_worker_throughput)
HM_TEST_SUITE_END()
