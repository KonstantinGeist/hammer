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

#include "array.h"
#include "allocator.h"

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
    char* items = (char*)hmAlloc(allocator, item_size*initial_capacity);
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
    if (array->item_dispose_func) {
        char* item = array->items;
        for (hm_nint i = 0; i < array->count; i++) {
            hmError err = array->item_dispose_func(item);
            if (err != HM_OK) {
                return HM_OK;
            }
            item += array->item_size;
        }
    }
    hmFree(array->allocator, array->items);
    return HM_OK;
}

hmError hmArrayAdd(hmArray* array, void* in_value)
{
    memcpy(array->items + array->count*array->item_size, in_value, array->item_size);
    array->count++;
    if (array->count >= array->capacity) {
        hm_nint new_capacity = array->capacity*2;
        array->items = hmRealloc(
            array->allocator,
            array->items,
            array->item_size*array->capacity,
            array->item_size*new_capacity);
        if (!array->items) {
            return HM_ERROR_OUT_OF_MEMORY;
        }
        array->capacity = new_capacity;
    }
    return HM_OK;
}

hmError hmArrayGet(hmArray* array, hm_nint index, void* in_value)
{
    if (index >= array->count) {
        return HM_ERROR_OUT_OF_RANGE;
    }
    memcpy(in_value, array->items + index*array->item_size, array->item_size);
    return HM_OK;
}

hmError hmArraySet(hmArray* array, hm_nint index, void* in_value)
{
    if (index >= array->count) {
        return HM_ERROR_OUT_OF_RANGE;
    }
    memcpy(array->items + index*array->item_size, in_value, array->item_size);
    return HM_OK;
}

hmError hmArrayExpand(hmArray* array, hm_nint count, hmArrayExpandFunc array_expand_func, void* user_data)
{
    if (!count) {
        return HM_OK;
    }
    hm_nint new_count = array->count+count;
    if (new_count > array->capacity) {
        hm_nint new_capacity = array->capacity+count;
        array->items = hmRealloc(
            array->allocator,
            array->items,
            array->item_size*array->capacity,
            array->item_size*new_capacity);
        if (!array->items) {
            return HM_ERROR_OUT_OF_MEMORY;
        }
        array->capacity = new_capacity;
    }
    if (array_expand_func) {
        char* item = array->items+array->count*array->item_size;
        for (hm_nint i = 0; i < count; i++) {
            hmError err = array_expand_func(array, array->count+i, item, user_data);
            if (err != HM_OK) {
                return err; // NOTE: no need to deallocate array->items on error
            }
            item += array->item_size;
        }
    } else {
        memset(array->items+array->count*array->item_size, 0, array->item_size*count);
    }
    array->count = new_count;
    return HM_OK;
}
