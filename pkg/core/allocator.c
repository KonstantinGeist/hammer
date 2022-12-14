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

#include <core/allocator.h>
#include <core/math.h>
#include <core/utils.h>

#include <stdlib.h> /* for malloc(..) and free(..) */

void* hmAlloc(hmAllocator* allocator, hm_nint sz)
{
    if (!sz) {
        return HM_NULL; /* it's meaningless to try allocate 0 bytes */
    }
    return allocator->alloc(allocator, hmAlignSize(sz));
}

void* hmAllocZeroInitialized(hmAllocator* allocator, hm_nint sz)
{
    if (!sz) {
        return HM_NULL; /* it's meaningless to try allocate 0 bytes */
    }
    sz = hmAlignSize(sz);
    void* r = allocator->alloc(allocator, sz);
    if (r) {
        hmZeroMemory(r, sz);
    }
    return r;
}

void* hmRealloc(hmAllocator* allocator, void* mem, hm_nint old_size, hm_nint new_size)
{
    new_size = hmAlignSize(new_size);
    if (new_size <= old_size) {
        return mem;
    }
    void* new_mem = allocator->alloc(allocator, new_size);
    if (!new_mem) {
        return HM_NULL;
    }
    if (mem) {
        hmCopyMemory(new_mem, mem, old_size);
        allocator->free(allocator, mem);
    }
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
    /* Do nothing (everything's managed by the C runtime). */
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

#define HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE (256*1024) /* 256KB */
#define HM_LARGE_OBJECT_SIZE_THRESHOLD (HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE/2)

typedef struct _hmBumpPointerAllocatorSegment {
    struct _hmBumpPointerAllocatorSegment* next;

    hm_nint index;
    char    data[1];
} hmBumpPointerAllocatorSegment;

typedef struct {
    hmBumpPointerAllocatorSegment* cur_segment;

    hmAllocator* base_allocator;
    void**       large_objects; /* for objects larger than HM_LARGE_OBJECT_SIZE_THRESHOLD */
    hm_nint      large_object_count;
    hm_nint      memory_limit;
    hm_nint      used_memory;
} hmBumpPointerAllocatorData;

static void* hmBumpPointerAllocator_alloc(hmAllocator* allocator, hm_nint sz)
{
    hmBumpPointerAllocatorData* data = (hmBumpPointerAllocatorData*)allocator->data;
    hm_nint new_used_memory = 0;
    hmError err = hmAddNint(data->used_memory, sz, &new_used_memory);
    if (err != HM_OK || new_used_memory > data->memory_limit) {
        return HM_NULL;
    }
    if (sz > HM_LARGE_OBJECT_SIZE_THRESHOLD) { /* too large to fit in a segment */
        void* result = hmAlloc(data->base_allocator, sz);
        if (!result) {
            return HM_NULL;
        }
        hm_nint new_large_object_count = 0, new_large_objects_size = 0;
        err = hmAddNint(data->large_object_count, 1, &new_large_object_count);
        err = hmMergeErrors(err, hmMulNint(sizeof(char*), new_large_object_count, &new_large_objects_size));
        if (err != HM_OK) {
            hmFree(data->base_allocator, result);
            return HM_NULL;
        }
        data->large_objects = hmRealloc(
            data->base_allocator,
            data->large_objects,
            sizeof(char*) * data->large_object_count, /* no safe math operations because it's an old value which was already prevalidated */
            new_large_objects_size
        );
        if (!data->large_objects) {
            hmFree(data->base_allocator, result);
            return HM_NULL;
        }
        data->large_objects[data->large_object_count] = result;
        data->large_object_count = new_large_object_count;
        data->used_memory = new_used_memory;
        return result;
    }
    hmBumpPointerAllocatorSegment* cur_segment = data->cur_segment;
    hm_nint new_index = 0;
    if (cur_segment) {
        err = hmAddNint(cur_segment->index, sz, &new_index);
        if (err != HM_OK) {
            return HM_NULL;
        }
    }
    if (!cur_segment || new_index > HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE) {
        hm_nint full_segment_size = sizeof(hmBumpPointerAllocatorSegment) + HM_BUMP_POINTER_ALLOCATOR_SEGMENT_SIZE - 1;
        hmBumpPointerAllocatorSegment* new_segment = hmAlloc(data->base_allocator, full_segment_size);
        if (!new_segment) {
            return HM_NULL;
        }
        new_segment->next = cur_segment;
        new_segment->index = 0;
        cur_segment = new_segment;
        data->cur_segment = cur_segment;
    }
    void* result = cur_segment->data + cur_segment->index; /* no need for overflow-safe math because already validated */
    cur_segment->index = new_index;
    data->used_memory = new_used_memory;
    return result;
}

static void hmBumpPointerAllocator_free(hmAllocator* allocator, void* mem)
{
    /* Do nothing. */
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

hmError hmCreateBumpPointerAllocator(hmAllocator* base_allocator, hm_nint memory_limit, hmAllocator* in_allocator)
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
    data->memory_limit = memory_limit;
    data->used_memory = 0;
    in_allocator->alloc = &hmBumpPointerAllocator_alloc;
    in_allocator->free = &hmBumpPointerAllocator_free;
    in_allocator->dispose = &hmBumpPointerAllocator_dispose;
    in_allocator->data = data;
    return HM_OK;
}

/* *********************** */
/*      StatsAllocator.   */
/* *********************** */

typedef struct {
    hmAllocator* base_allocator;
    hm_nint      total_alloc_count;
    hm_bool      is_tracking;
} hmStatsAllocatorData;

static void* hmStatsAllocator_alloc(hmAllocator* allocator, hm_nint sz)
{
    hmStatsAllocatorData* data = (hmStatsAllocatorData*)allocator->data;
    void* result = hmAlloc(data->base_allocator, sz);
    if (data->is_tracking) {
        /* In case of an overflow, total_alloc_count will simply stop updating. */
        hmAddNint(data->total_alloc_count, 1, &data->total_alloc_count);
    }
    return result;
}

static void hmStatsAllocator_free(hmAllocator* allocator, void* mem)
{
    hmStatsAllocatorData* data = (hmStatsAllocatorData*)allocator->data;
    hmFree(data->base_allocator, mem);
}

static hmError hmStatsAllocator_dispose(hmAllocator* allocator)
{
    hmStatsAllocatorData* data = (hmStatsAllocatorData*)allocator->data;
    hmAllocator* base_allocator = data->base_allocator;
    hmFree(base_allocator, data);
    return hmAllocatorDispose(base_allocator);
}

hmError hmCreateStatsAllocator(hmAllocator* base_allocator, hmAllocator* in_allocator)
{
    if (!base_allocator || !in_allocator) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmStatsAllocatorData* data = hmAlloc(base_allocator, sizeof(hmStatsAllocatorData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    data->base_allocator = base_allocator;
    data->total_alloc_count = 0;
    data->is_tracking = HM_TRUE;
    in_allocator->alloc = &hmStatsAllocator_alloc;
    in_allocator->free = &hmStatsAllocator_free;
    in_allocator->dispose = &hmStatsAllocator_dispose;
    in_allocator->data = data;
    return HM_OK;
}

hm_nint hmStatsAllocatorGetTotalCount(hmAllocator* allocator)
{
    hmStatsAllocatorData* data = (hmStatsAllocatorData*)allocator->data;
    return data->total_alloc_count;
}

void hmStatsAllocatorTrackAllocCount(hmAllocator* allocator, hm_bool value)
{
    hmStatsAllocatorData* data = (hmStatsAllocatorData*)allocator->data;
    data->is_tracking = value;
}

/* ********************** */
/*      OOMAllocator.    */
/* ********************* */

typedef struct {
    hmAllocator* base_allocator;
    hm_nint      total_alloc_count;
    hm_nint      failed_alloc_number;
    hm_bool      is_tracking;
} hmOOMAllocatorData;

static void* hmOOMAllocator_alloc(hmAllocator* allocator, hm_nint sz)
{
    hmOOMAllocatorData* data = (hmOOMAllocatorData*)allocator->data;
    if (data->is_tracking && data->total_alloc_count >= data->failed_alloc_number) {
        return HM_NULL;
    }
    void* result = hmAlloc(data->base_allocator, sz);
    if (data->is_tracking) {
        /* In case of an overflow, total_alloc_count will simply stop updating. */
        hmAddNint(data->total_alloc_count, 1, &data->total_alloc_count);
    }
    return result;
}

static void hmOOMAllocator_free(hmAllocator* allocator, void* mem)
{
    hmOOMAllocatorData* data = (hmOOMAllocatorData*)allocator->data;
    hmFree(data->base_allocator, mem);
}

static hmError hmOOMAllocator_dispose(hmAllocator* allocator)
{
    hmOOMAllocatorData* data = (hmOOMAllocatorData*)allocator->data;
    hmAllocator* base_allocator = data->base_allocator;
    hmFree(base_allocator, data);
    return hmAllocatorDispose(base_allocator);
}

hmError hmCreateOOMAllocator(hmAllocator* base_allocator, hm_nint failed_alloc_number, hmAllocator* in_allocator)
{
    if (!base_allocator || !in_allocator) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmOOMAllocatorData* data = hmAlloc(base_allocator, sizeof(hmOOMAllocatorData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    data->base_allocator = base_allocator;
    data->total_alloc_count = 0;
    data->failed_alloc_number = failed_alloc_number;
    data->is_tracking = HM_TRUE;
    in_allocator->alloc = &hmOOMAllocator_alloc;
    in_allocator->free = &hmOOMAllocator_free;
    in_allocator->dispose = &hmOOMAllocator_dispose;
    in_allocator->data = data;
    return HM_OK;
}

hm_nint hmOOMAllocatorIsOutOfMEmory(hmAllocator* allocator)
{
    hmOOMAllocatorData* data = (hmOOMAllocatorData*)allocator->data;
    return data->total_alloc_count >= data->failed_alloc_number;
}

void hmOOMAllocatorTrackAllocCount(hmAllocator* allocator, hm_bool value)
{
    hmOOMAllocatorData* data = (hmOOMAllocatorData*)allocator->data;
    data->is_tracking = value;
}

/* ************************ */
/*      BufferAllocator.    */
/* ************************ */

typedef struct {
    char*        start;
    char*        current;
    char*        end;
    hmAllocator* fallback_allocator;
} hmBufferAllocatorData;

static void* hmBufferAllocator_alloc(hmAllocator* allocator, hm_nint sz)
{
    hmBufferAllocatorData* data = (hmBufferAllocatorData*)allocator->data;
    if (sz > data->end - data->current) { /* underflow-safe because `end` must be greater than `current` */
        if (data->fallback_allocator) {
            return hmAlloc(data->fallback_allocator, sz);
        }
        return HM_NULL;
    }
    char* result = data->current;
    hm_nint new_current = 0;
    hmError err = hmAddNint(hmCastPointerToNint(data->current), sz, &new_current);
    if (err != HM_OK) {
        /* Tries to use the fallback allocator on overflow: the only sane behavior at this point. */
        return hmAlloc(data->fallback_allocator, sz);
    }
    data->current = hmCastNintToPointer(new_current, char*);
    return result;
}

static void hmBufferAllocator_free(hmAllocator* allocator, void* mem)
{
    hmBufferAllocatorData* data = (hmBufferAllocatorData*)allocator->data;
    if ((char*)mem >= data->start && (char*)mem < data->end) {
        /* Nothing to do: allocated from the supplied buffer. */
        return;
    }
    if (data->fallback_allocator) {
        /* Otherwise, deallocate with the fallback allocator. */
        hmFree(data->fallback_allocator, mem);
    }
}

static hmError hmBufferAllocator_dispose(hmAllocator* allocator)
{
    /* Nothing to do. */
    return HM_OK;
}

hmError hmCreateBufferAllocator(char* buffer, hm_nint buffer_size, hmAllocator* fallback_allocator, hmAllocator* in_allocator)
{
    if (!buffer || buffer_size < sizeof(hmBufferAllocatorData) + HM_ALLOC_SIZE_ALIGNMENT) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmBufferAllocatorData* data = (hmBufferAllocatorData*)buffer;
    hm_nint current = 0, end = 0;
    HM_TRY(hmAddNint(hmCastPointerToNint(buffer), sizeof(hmBufferAllocatorData), &current));
    HM_TRY(hmAddNint(hmCastPointerToNint(buffer), buffer_size, &end));
    data->start = buffer;
    data->current = hmCastNintToPointer(current, char*);
    data->end = hmCastNintToPointer(end, char*);
    data->fallback_allocator = fallback_allocator;
    in_allocator->data = data;
    in_allocator->alloc = &hmBufferAllocator_alloc;
    in_allocator->free = &hmBufferAllocator_free;
    in_allocator->dispose = &hmBufferAllocator_dispose;
    return HM_OK;
}
