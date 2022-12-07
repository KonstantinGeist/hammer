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

#include <collections/array.h>
#include <core/allocator.h>
#include <core/math.h>

#define HM_ARRAY_GROWTH_FACTOR 2

hmError hmCreateArray(
    struct _hmAllocator* allocator,
    hm_nint item_size,
    hm_nint initial_capacity,
    hmDisposeFunc item_dispose_func,
    hmArray* in_array)
{
    if (!item_size || !initial_capacity) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hm_nint items_capacity = 0;
    HM_TRY(hmMulNint(item_size, initial_capacity, &items_capacity));
    char* items = (char*)hmAlloc(allocator, items_capacity);
    if (!items) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    in_array->items = items;
    in_array->allocator = allocator;
    in_array->item_dispose_func = item_dispose_func;
    in_array->item_size = item_size;
    in_array->capacity = initial_capacity;
    in_array->count = 0;
    return HM_OK;
}

hmError hmArrayDispose(hmArray* array)
{
    hmError err = HM_OK;
    if (array->item_dispose_func) {
        char* item = array->items;
        for (hm_nint i = 0; i < array->count; i++) {
            err = hmMergeErrors(err, array->item_dispose_func(item));
            /* No hmAddNint because if we were able to add this many elements, it must have been valid. */
            item += array->item_size;
        }
    }
    hmFree(array->allocator, array->items);
    return err;
}

hmError hmArrayAdd(hmArray* array, void* in_value)
{
    hm_nint new_count = 0;
    HM_TRY(hmAddNint(array->count, 1, &new_count));
    if (new_count >= array->capacity) {
        hm_nint new_capacity = 0;
        HM_TRY(hmMulNint(array->capacity, HM_ARRAY_GROWTH_FACTOR, &new_capacity));
        hm_nint new_items_capacity = 0;
        HM_TRY(hmMulNint(array->item_size, new_capacity, &new_items_capacity));
        char* new_items = hmRealloc(
            array->allocator,
            array->items,
            array->item_size * array->capacity, /* No hmMulNint here, because it's an old value which was already validated. */
            new_items_capacity
        );
        if (!new_items) {
            return HM_ERROR_OUT_OF_MEMORY;
        }
        array->items = new_items;
        array->capacity = new_capacity;
    }
    hm_nint item_address = 0;
    HM_TRY(hmAddMulNint((hm_nint)array->items, array->count, array->item_size, &item_address));
    memcpy((char*)item_address, in_value, array->item_size);
    array->count = new_count;
    return HM_OK;
}

hmError hmArrayGet(hmArray* array, hm_nint index, void* in_value)
{
    if (index >= array->count) {
        return HM_ERROR_OUT_OF_RANGE;
    }
    hm_nint item_address = 0;
    HM_TRY(hmAddMulNint((hm_nint)array->items, index, array->item_size, &item_address));
    memcpy(in_value, (char*)item_address, array->item_size);
    return HM_OK;
}

hmError hmArraySet(hmArray* array, hm_nint index, void* in_value)
{
    if (index >= array->count) {
        return HM_ERROR_OUT_OF_RANGE;
    }
    hm_nint item_address = 0;
    HM_TRY(hmAddMulNint((hm_nint)array->items, index, array->item_size, &item_address));
    memcpy((char*)item_address, in_value, array->item_size);
    return HM_OK;
}

hmError hmArrayExpand(hmArray* array, hm_nint count, hmArrayExpandFunc array_expand_func, void* user_data)
{
    if (!count) {
        return HM_OK;
    }
    hm_nint new_count = 0;
    HM_TRY(hmAddNint(array->count, count, &new_count));
    if (new_count >= array->capacity) {
        hm_nint new_capacity = 0;
        HM_TRY(hmAddNint(array->capacity, count, &new_capacity));
        hm_nint new_items_capacity = 0;
        HM_TRY(hmMulNint(array->item_size, new_capacity, &new_items_capacity));
        char* new_items = hmRealloc(
            array->allocator,
            array->items,
            array->item_size * array->capacity, /* No hmMulNint here, because it's an old value which was already validated. */
            new_items_capacity
        );
        if (!new_items) {
            return HM_ERROR_OUT_OF_MEMORY;
        }
        array->items = new_items;
        array->capacity = new_capacity;
    }
    /* The following block doesn't need safe math operations, because they were prevalidated when expanding the internal buffer. */
    if (array_expand_func) {
        char* item = array->items+array->count * array->item_size;
        for (hm_nint i = 0; i < count; i++) {
            /* NOTE: no need to deallocate array->items on error */
            HM_TRY(array_expand_func(array, array->count + i, item, user_data));
            item += array->item_size;
        }
    } else {
        memset(array->items + array->count * array->item_size, 0, array->item_size * count);
    }
    array->count = new_count;
    return HM_OK;
}
