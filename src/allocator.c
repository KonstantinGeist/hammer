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

#include "allocator.h"
#include "utils.h"

void* hmAlloc(hmAllocator* allocator, hm_nint sz)
{
    return allocator->alloc(allocator, sz);
}

void* hmAllocZeroInitialized(hmAllocator* allocator, hm_nint sz)
{
    void* r = allocator->alloc(allocator, sz);
    if (r) {
        memset(r, 0, sz);
    }
    return r;
}

void* hmRealloc(hmAllocator* allocator, void* mem, hm_nint old_size, hm_nint new_size)
{
    if (new_size <= old_size) {
        return mem;
    }
    void* new_mem = allocator->alloc(allocator, new_size);
    if (!new_mem) {
        return HM_NULL;
    }
    memcpy(new_mem, mem, old_size);
    allocator->free(allocator, mem);
    return new_mem;
}

void hmFree(hmAllocator* allocator, void* mem)
{
    allocator->free(allocator, mem);
}

hmError hmAllocatorDispose(hmAllocator* allocator)
{
    return allocator->dispose(allocator);
}

/* ********************** */
/*    SystemAllocator.    */
/* ********************** */

static void* hmSystemAllocator_alloc(hmAllocator* allocator, hm_nint sz)
{
    return malloc(sz);
}

static void hmSystemAllocator_free(hmAllocator* allocator, void* mem)
{
    free(mem);
}

static hmError hmSystemAllocator_dispose(hmAllocator* allocator)
{
    // Do nothing (everything's managed by the OS).
    return HM_OK;
}

hmError hmCreateSystemAllocator(hmAllocator* in_allocator)
{
    if (!in_allocator) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    in_allocator->alloc = &hmSystemAllocator_alloc;
    in_allocator->free = &hmSystemAllocator_free;
    in_allocator->dispose = &hmSystemAllocator_dispose;
    in_allocator->data = HM_NULL;
    return HM_OK;
}

/* *************************** */
/*    BumpPointerAllocator.    */
/* *************************** */

#define HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE (1024*1024*4) // 4 MB

typedef struct _hmBumpPointerAllocatorSegment {
    struct _hmBumpPointerAllocatorSegment* next;

    hm_nint index;
    char    data[1];
} hmBumpPointerAllocatorSegment;

typedef struct {
    hmBumpPointerAllocatorSegment* cur_segment;

    hmAllocator* base_allocator;
    void**       large_objects; // for objects larger than HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE
    hm_nint      large_object_count;
} hmBumpPointerAllocatorData;

static void* hmBumpPointerAllocator_alloc(hmAllocator* allocator, hm_nint sz)
{
    hmBumpPointerAllocatorData* data = (hmBumpPointerAllocatorData*)allocator->data;
    if (sz > HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE) { // too large to fit in a segment
        void* result = hmAlloc(data->base_allocator, sz);
        if (!result) {
            return HM_NULL;
        }
        data->large_objects = hmRealloc(
            data->base_allocator,
            data->large_objects,
            sizeof(char*)*data->large_object_count,
            sizeof(char*)*(data->large_object_count+1)
        );
        if (!data->large_objects) {
            hmFree(data->base_allocator, result);
            return HM_NULL;
        }
        data->large_objects[data->large_object_count] = result;
        data->large_object_count++;
        return result;
    }
    sz = hmAlignSize(sz);
    hmBumpPointerAllocatorSegment* cur_segment = data->cur_segment;
    if (!cur_segment || cur_segment->index + sz > HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE) {
        hm_nint full_segment_size = sizeof(hmBumpPointerAllocatorSegment)+HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE-1;
        hmBumpPointerAllocatorSegment* new_segment = hmAlloc(data->base_allocator, full_segment_size);
        if (!new_segment) {
            return HM_NULL;
        }
        new_segment->next = cur_segment;
        new_segment->index = 0;
        cur_segment = new_segment;
        data->cur_segment = cur_segment;
    }
    void* result = &cur_segment->data[0] + cur_segment->index;
    cur_segment->index += sz;
    return result;
}

static void hmBumpPointerAllocator_free(hmAllocator* allocator, void* mem)
{
    // Do nothing.
}

static hmError hmBumpPointerAllocator_dispose(hmAllocator* allocator)
{
    hmBumpPointerAllocatorData* data = (hmBumpPointerAllocatorData*)allocator->data;
    hmBumpPointerAllocatorSegment* cur_segment = data->cur_segment;
    while (cur_segment) {
        hmBumpPointerAllocatorSegment* next_segment = cur_segment->next;
        hmFree(data->base_allocator, cur_segment);
        cur_segment = next_segment;
    }
    for (hm_nint i = 0; i < data->large_object_count; i++) {
        hmFree(data->base_allocator, data->large_objects[i]);
    }
    hmFree(data->base_allocator, data->large_objects);
    hmFree(data->base_allocator, data);
    return HM_OK;
}

hmError hmCreateBumpPointerAllocator(hmAllocator* base_allocator, hmAllocator* in_allocator)
{
    if (!base_allocator || !in_allocator) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmBumpPointerAllocatorData* data = hmAlloc(base_allocator, sizeof(hmBumpPointerAllocatorData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    data->cur_segment = HM_NULL;
    data->base_allocator = base_allocator;
    data->large_objects = HM_NULL;
    data->large_object_count = 0;
    in_allocator->alloc = &hmBumpPointerAllocator_alloc;
    in_allocator->free = &hmBumpPointerAllocator_free;
    in_allocator->dispose = &hmBumpPointerAllocator_dispose;
    in_allocator->data = data;
    return HM_OK;
}
