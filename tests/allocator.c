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
#include "../src/allocator.h"

static void test_can_alloc_realloc_and_free_from_system_allocator()
{
    #define MEM_BLOCK_SENTINEL 13
    #define NEW_MEM_BLOCK_SENTINEL 14
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    for (hm_nint i = 0; i < 100; i++) {
        hm_nint mem_size = i;
        hm_nint new_mem_size = i*2;
        void* mem = hmAlloc(&allocator, mem_size);
        HM_TEST_ASSERT(mem != HM_NULL);
        memset(mem, MEM_BLOCK_SENTINEL, mem_size); // to make sure Valgrind touches the bytes and reports problems if any
        void* new_mem = hmRealloc(&allocator, mem, mem_size, new_mem_size);
        HM_TEST_ASSERT(new_mem != HM_NULL);
        for (hm_nint j = 0; j < mem_size; j++) {
            HM_TEST_ASSERT(((hm_uint8*)new_mem)[j] == (hm_uint8)MEM_BLOCK_SENTINEL);
        }
        memset(new_mem, NEW_MEM_BLOCK_SENTINEL, new_mem_size); // to make sure Valgrind touches the bytes and reports problems if any
        hmFree(&allocator, new_mem);
    }
    hmAllocatorDispose(&allocator);
}

void test_allocator()
{
    test_can_alloc_realloc_and_free_from_system_allocator();
}
