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
#include <core/allocator.h>
#include <core/utils.h>

#include <string.h> /* for memset(..) */

#define MEM_BLOCK_SENTINEL 13
#define NEW_MEM_BLOCK_SENTINEL 14

#define BUFFER_ALLOCATOR_BUFFER_SIZE 1024
#define BUFFER_ALLOCATOR_ALLOCATION_COUNT 4

#define BUMP_POINTER_ALLOCATOR_LIMIT_SIZE (124*1024*1024)

static void create_system_allocator(hmAllocator* allocator)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void create_bump_pointer_allocator(hmAllocator* system_allocator, hm_nint memory_limit, hmAllocator* bump_pointer_allocator)
{
    hmError err = hmCreateSystemAllocator(system_allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateBumpPointerAllocator(system_allocator, memory_limit, bump_pointer_allocator);
    HM_TEST_ASSERT_OK(err);
}

static void dispose_allocator(hmAllocator* allocator)
{
    hmError err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void touch_memory(void* mem, hm_nint mem_size)
{
    memset(mem, MEM_BLOCK_SENTINEL, mem_size); /* to make sure Valgrind touches the bytes and reports problems if any */
}

static void test_can_alloc_realloc_and_free_from_allocator(hmAllocator* allocator)
{
    for (hm_nint i = 1; i < 100; i++) {
        hm_nint mem_size = i;
        hm_nint new_mem_size = i*2;
        void* mem = hmAlloc(allocator, mem_size);
        HM_TEST_ASSERT(mem != HM_NULL);
        touch_memory(mem, mem_size);
        void* new_mem = hmRealloc(allocator, mem, mem_size, new_mem_size);
        HM_TEST_ASSERT(new_mem != HM_NULL);
        for (hm_nint j = 0; j < mem_size; j++) {
            HM_TEST_ASSERT(((hm_uint8*)new_mem)[j] == (hm_uint8)MEM_BLOCK_SENTINEL);
        }
        touch_memory(new_mem, new_mem_size);
        hmFree(allocator, new_mem);
    }
}

static void test_can_alloc_realloc_and_free_from_system_allocator()
{
    hmAllocator allocator;
    create_system_allocator(&allocator);
    test_can_alloc_realloc_and_free_from_allocator(&allocator);
    dispose_allocator(&allocator);
}

static void test_can_alloc_realloc_and_free_from_bump_pointer_allocator()
{
    hmAllocator system_allocator;
    hmAllocator bump_pointer_allocator;
    create_bump_pointer_allocator(&system_allocator, BUMP_POINTER_ALLOCATOR_LIMIT_SIZE, &bump_pointer_allocator);
    test_can_alloc_realloc_and_free_from_allocator(&bump_pointer_allocator);
    dispose_allocator(&bump_pointer_allocator);
    dispose_allocator(&system_allocator);
}

static void test_realloc_accepts_smaller_size()
{
    hmAllocator allocator;
    create_system_allocator(&allocator);
    void* mem = hmAlloc(&allocator, 100);
    HM_TEST_ASSERT(mem != HM_NULL);
    mem = hmRealloc(&allocator, mem, 100, 50);
    hmFree(&allocator, mem);
    dispose_allocator(&allocator);
}

static void test_bump_pointer_allocator_works_with_large_objects()
{
    hmAllocator system_allocator;
    hmAllocator bump_pointer_allocator;
    create_bump_pointer_allocator(&system_allocator, BUMP_POINTER_ALLOCATOR_LIMIT_SIZE, &bump_pointer_allocator);
    void* mems[3] = {0};
    for (hm_nint i = 0; i < 3; i++) {
        int size_to_allocate = 4*1024*1023+i;
        void* mem = hmAlloc(&bump_pointer_allocator, size_to_allocate);
        HM_TEST_ASSERT(mem != HM_NULL);
        touch_memory(mem, size_to_allocate);
        mems[i] = mem;
    }
    for (hm_nint i = 0; i < 3; i++)
    {
        hmFree(&bump_pointer_allocator, mems[i]);
    }
    dispose_allocator(&bump_pointer_allocator);
    dispose_allocator(&system_allocator);
}

static void test_stats_allocator_keeps_track_of_alloc_count()
{
    hmAllocator system_allocator;
    hmAllocator stats_allocator;
    hmError err = hmCreateSystemAllocator(&system_allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateStatsAllocator(&system_allocator, &stats_allocator);
    HM_TEST_ASSERT_OK(err);
    void* obj1 = hmAlloc(&stats_allocator, sizeof(hm_nint));
    hm_nint alloc_count = hmStatsAllocatorGetTotalCount(&stats_allocator);
    HM_TEST_ASSERT(alloc_count == 1);
    void* obj2 = hmAlloc(&stats_allocator, sizeof(hm_uint8));
    alloc_count = hmStatsAllocatorGetTotalCount(&stats_allocator);
    HM_TEST_ASSERT(alloc_count == 2);
    hmFree(&stats_allocator, obj1);
    hmFree(&stats_allocator, obj2);
    dispose_allocator(&stats_allocator);
    dispose_allocator(&system_allocator);
}

static void test_oom_allocator_returns_out_of_memory()
{
    hmAllocator system_allocator;
    hmAllocator oom_allocator;
    hmError err = hmCreateSystemAllocator(&system_allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateOOMAllocator(&system_allocator, 1, &oom_allocator);
    HM_TEST_ASSERT_OK(err);
    void* obj1 = hmAlloc(&oom_allocator, sizeof(hm_nint));
    HM_TEST_ASSERT(obj1 != HM_NULL);
    void* obj2 = hmAlloc(&oom_allocator, sizeof(hm_uint8));
    HM_TEST_ASSERT(obj2 == HM_NULL);
    hmFree(&oom_allocator, obj1);
    hmFree(&oom_allocator, obj2);
    dispose_allocator(&oom_allocator);
    dispose_allocator(&system_allocator);
}

static void test_can_allocate_from_buffer_allocator()
{
    hmAllocator allocator;
    char buffer[BUFFER_ALLOCATOR_BUFFER_SIZE + HM_BUFFER_ALLOCATOR_INTERNAL_STATE_SIZE];
    hmError err = hmCreateBufferAllocator(
        buffer,
        BUFFER_ALLOCATOR_BUFFER_SIZE + HM_BUFFER_ALLOCATOR_INTERNAL_STATE_SIZE,
        HM_NULL,
        &allocator
    );
    HM_TEST_ASSERT_OK(err);
    void* values[BUFFER_ALLOCATOR_ALLOCATION_COUNT];
    for (hm_nint i = 0; i < BUFFER_ALLOCATOR_ALLOCATION_COUNT; i++) {
        hm_nint size_to_allocate = BUFFER_ALLOCATOR_BUFFER_SIZE / BUFFER_ALLOCATOR_ALLOCATION_COUNT;
        void* mem = hmAlloc(&allocator, size_to_allocate);
        HM_TEST_ASSERT(mem != HM_NULL);
        touch_memory(mem, size_to_allocate);
        values[i] = mem;
    }
    for (hm_nint i = 0; i < BUFFER_ALLOCATOR_ALLOCATION_COUNT; i++) {
        hmFree(&allocator, values[i]);
    }
    dispose_allocator(&allocator);
}

static void test_buffer_allocator_returns_out_of_memory()
{
    hmAllocator allocator;
    char buffer[BUFFER_ALLOCATOR_BUFFER_SIZE + HM_BUFFER_ALLOCATOR_INTERNAL_STATE_SIZE];
    hmError err = hmCreateBufferAllocator(
        buffer,
        BUFFER_ALLOCATOR_BUFFER_SIZE + HM_BUFFER_ALLOCATOR_INTERNAL_STATE_SIZE,
        HM_NULL,
        &allocator
    );
    HM_TEST_ASSERT_OK(err);
    void* values[BUFFER_ALLOCATOR_ALLOCATION_COUNT + 1];
    for (hm_nint i = 0; i < BUFFER_ALLOCATOR_ALLOCATION_COUNT + 1; i++) {
        hm_bool is_oom_iteration = i >= BUFFER_ALLOCATOR_ALLOCATION_COUNT;
        hm_nint size_to_allocate = BUFFER_ALLOCATOR_BUFFER_SIZE / BUFFER_ALLOCATOR_ALLOCATION_COUNT;
        if (is_oom_iteration) {
            size_to_allocate += 20;
        }
        void* mem = hmAlloc(
            &allocator,
            BUFFER_ALLOCATOR_BUFFER_SIZE / BUFFER_ALLOCATOR_ALLOCATION_COUNT
        );
        if (is_oom_iteration) {
            HM_TEST_ASSERT(mem == HM_NULL);
        } else {
            HM_TEST_ASSERT(mem != HM_NULL);
            touch_memory(mem, size_to_allocate);
        }
        values[i] = mem;
    }
    for (hm_nint i = 0; i < BUFFER_ALLOCATOR_ALLOCATION_COUNT; i++) {
        hmFree(&allocator, values[i]);
    }
    dispose_allocator(&allocator);
}

static void test_buffer_allocator_uses_fallback_allocator_when_out_of_memory()
{
    hmAllocator fallback_allocator;
    create_system_allocator(&fallback_allocator);
    hmAllocator allocator;
    char buffer[BUFFER_ALLOCATOR_BUFFER_SIZE + HM_BUFFER_ALLOCATOR_INTERNAL_STATE_SIZE];
    hmError err = hmCreateBufferAllocator(
        buffer,
        BUFFER_ALLOCATOR_BUFFER_SIZE + HM_BUFFER_ALLOCATOR_INTERNAL_STATE_SIZE,
        &fallback_allocator,
        &allocator
    );
    HM_TEST_ASSERT_OK(err);
    void* values[BUFFER_ALLOCATOR_ALLOCATION_COUNT + 1];
    for (hm_nint i = 0; i < BUFFER_ALLOCATOR_ALLOCATION_COUNT + 1; i++) {
        hm_nint size_to_allocate = BUFFER_ALLOCATOR_BUFFER_SIZE / BUFFER_ALLOCATOR_ALLOCATION_COUNT;
        void* mem = hmAlloc(&allocator, size_to_allocate);
        HM_TEST_ASSERT(mem != HM_NULL);
        touch_memory(mem, size_to_allocate);
        values[i] = mem;
    }
    for (hm_nint i = 0; i < BUFFER_ALLOCATOR_ALLOCATION_COUNT + 1; i++) {
        hmFree(&allocator, values[i]);
    }
    dispose_allocator(&allocator);
    dispose_allocator(&fallback_allocator);
}

static void test_can_alloc_zero_initialized()
{
    hmAllocator allocator;
    create_system_allocator(&allocator);
    hm_nint size = 16;
    char* mem = (char*)hmAllocZeroInitialized(&allocator, size);
    HM_TEST_ASSERT(mem != HM_NULL);
    for (hm_nint i = 0; i < size; i++) {
        HM_TEST_ASSERT(mem[i] == 0);
    }
    hmFree(&allocator, mem);
    dispose_allocator(&allocator);
}

static void test_alloc_returns_aligned_memory()
{
    hmAllocator allocator;
    create_system_allocator(&allocator);
    hm_nint size = 24;
    hm_nint aligned_size = hmAlignSize(size);
    char* mem = (char*)hmAllocZeroInitialized(&allocator, size);
    HM_TEST_ASSERT(mem != HM_NULL);
    for (hm_nint i = 0; i < aligned_size; i++) {
        HM_TEST_ASSERT(mem[i] == 0); /* we hope Valgrind will report problems if it's past the array */
    }
    hmFree(&allocator, mem);
    dispose_allocator(&allocator);
}

static void test_bump_pointer_limits_memory_size()
{
    hmAllocator system_allocator;
    hmAllocator bump_pointer_allocator;
    create_bump_pointer_allocator(&system_allocator, 1064, &bump_pointer_allocator);
    void* mem = hmAlloc(&bump_pointer_allocator, 1024);
    HM_TEST_ASSERT(mem != HM_NULL);
    hmFree(&bump_pointer_allocator, mem);
    mem = hmAlloc(&bump_pointer_allocator, 32);
    HM_TEST_ASSERT(mem != HM_NULL);
    hmFree(&bump_pointer_allocator, mem);
    mem = hmAlloc(&bump_pointer_allocator, 32);
    HM_TEST_ASSERT(mem == HM_NULL);
    dispose_allocator(&bump_pointer_allocator);
    dispose_allocator(&system_allocator);
}

static void test_realloc_on_null_behaves_like_alloc()
{
    hmAllocator allocator;
    create_system_allocator(&allocator);
    hm_nint size = 16;
    char* mem = (char*)hmRealloc(&allocator, HM_NULL, 0, size);
    HM_TEST_ASSERT(mem != HM_NULL);
    touch_memory(mem, size);
    hmFree(&allocator, mem);
    dispose_allocator(&allocator);
}

HM_TEST_SUITE_BEGIN(allocators)
    HM_TEST_RUN_WITHOUT_OOM(test_can_alloc_realloc_and_free_from_system_allocator)
    HM_TEST_RUN_WITHOUT_OOM(test_can_alloc_realloc_and_free_from_bump_pointer_allocator)
    HM_TEST_RUN_WITHOUT_OOM(test_realloc_accepts_smaller_size)
    HM_TEST_RUN_WITHOUT_OOM(test_bump_pointer_allocator_works_with_large_objects)
    HM_TEST_RUN_WITHOUT_OOM(test_stats_allocator_keeps_track_of_alloc_count)
    HM_TEST_RUN_WITHOUT_OOM(test_oom_allocator_returns_out_of_memory)
    HM_TEST_RUN_WITHOUT_OOM(test_can_allocate_from_buffer_allocator)
    HM_TEST_RUN_WITHOUT_OOM(test_buffer_allocator_returns_out_of_memory)
    HM_TEST_RUN_WITHOUT_OOM(test_buffer_allocator_uses_fallback_allocator_when_out_of_memory)
    HM_TEST_RUN_WITHOUT_OOM(test_can_alloc_zero_initialized)
    HM_TEST_RUN_WITHOUT_OOM(test_alloc_returns_aligned_memory)
    HM_TEST_RUN_WITHOUT_OOM(test_bump_pointer_limits_memory_size)
    HM_TEST_RUN_WITHOUT_OOM(test_realloc_on_null_behaves_like_alloc)
HM_TEST_SUITE_END()
