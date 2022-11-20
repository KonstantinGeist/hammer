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

#include "common.h"
#include <core/allocator.h>

static void create_system_allocator(hmAllocator* allocator)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void create_bump_pointer_allocator(hmAllocator* system_allocator, hmAllocator* bump_pointer_allocator)
{
    hmError err = hmCreateSystemAllocator(system_allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateBumpPointerAllocator(system_allocator, bump_pointer_allocator);
    HM_TEST_ASSERT_OK(err);
}

static void dispose_allocator(hmAllocator* allocator)
{
    hmError err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_can_alloc_realloc_and_free_from_allocator(hmAllocator* allocator)
{
    #define MEM_BLOCK_SENTINEL 13
    #define NEW_MEM_BLOCK_SENTINEL 14
    for (hm_nint i = 1; i < 100; i++) {
        hm_nint mem_size = i;
        hm_nint new_mem_size = i*2;
        void* mem = hmAlloc(allocator, mem_size);
        HM_TEST_ASSERT(mem != HM_NULL);
        memset(mem, MEM_BLOCK_SENTINEL, mem_size); // to make sure Valgrind touches the bytes and reports problems if any
        void* new_mem = hmRealloc(allocator, mem, mem_size, new_mem_size);
        HM_TEST_ASSERT(new_mem != HM_NULL);
        for (hm_nint j = 0; j < mem_size; j++) {
            HM_TEST_ASSERT(((hm_uint8*)new_mem)[j] == (hm_uint8)MEM_BLOCK_SENTINEL);
        }
        memset(new_mem, NEW_MEM_BLOCK_SENTINEL, new_mem_size); // to make sure Valgrind touches the bytes and reports problems if any
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
    create_bump_pointer_allocator(&system_allocator, &bump_pointer_allocator);
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
    create_bump_pointer_allocator(&system_allocator, &bump_pointer_allocator);
    void* mems[3] = {0};
    for (hm_nint i = 0; i < 3; i++) {
        int size = 4*1024*1023+i;
        void* mem = hmAlloc(&bump_pointer_allocator, size);
        memset(mem, NEW_MEM_BLOCK_SENTINEL, size); // to make sure Valgrind touches the bytes and reports problems if any
        HM_TEST_ASSERT(mem != HM_NULL);
        mems[i] = mem;
    }
    for (hm_nint i = 0; i < 3; i++)
    {
        hmFree(&bump_pointer_allocator, mems[i]);
    }
    dispose_allocator(&bump_pointer_allocator);
    dispose_allocator(&system_allocator);
}

void test_allocators()
{
    HM_TEST_SUITE_BEGIN("Allocators");
        HM_TEST_RUN(test_can_alloc_realloc_and_free_from_system_allocator);
        HM_TEST_RUN(test_can_alloc_realloc_and_free_from_bump_pointer_allocator);
        HM_TEST_RUN(test_realloc_accepts_smaller_size);
        HM_TEST_RUN(test_bump_pointer_allocator_works_with_large_objects);
    HM_TEST_SUITE_END();
}
