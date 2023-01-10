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

#ifndef HM_ARRAY_H
#define HM_ARRAY_H

#include <core/common.h>

#define HM_ARRAY_DEFAULT_CAPACITY 16

struct _hmAllocator;

/* A general purpose array. */
typedef struct {
    struct _hmAllocator* allocator;
    char*                items;             /* The list of entries in the array, a contiguous memory block of inlined values.
                                               Typed to void* because that's how we achieve genericity. */
    hmDisposeFunc        item_dispose_func; /* Dispose function: called on every item on container destruction. Can be HM_NULL. */
    hm_nint              item_size;         /* Size of a single item (this is how we achieve genericity). */
    hm_nint              capacity;          /* The capacity of the array: determines how many elements can be added
                                               before the internal backing array is resized. */
    hm_nint              count;             /* The count of objects in this array. */
} hmArray;

/* Array expansion function. User_data is passed from hmArrayExpand(..) (see). */
typedef hmError (*hmArrayExpandFunc)(hmArray* array, hm_nint index, void* in_item, void* user_data);

/* Creates a new array. When calling hmArrayAdd in a loop, make sure initial_capacity is set to a correct value
   so that we don't have to reallocate too often. */
hmError hmCreateArray(
    struct _hmAllocator* allocator,
    hm_nint item_size,
    hm_nint initial_capacity,
    hmDisposeFunc item_dispose_func,
    hmArray* in_array
);
hmError hmArrayDispose(hmArray* array);
/* Adds a new value to the array. in_value must be a reference to the actual value. An object is stored in an array
   by having a shallow copy. in_value is a reference to the value (not the value itself). The value is copied
   to the array's internal backing array by using the item size supplied in the constructor. */
hmError hmArrayAdd(hmArray* array, void* in_value);
/* Same as calling hmArrayAdd(..) in a loop, except potentially more performant.
   `in_values` points to a contiguous array of elements (values, not pointers). */
hmError hmArrayAddRange(hmArray* array, void* in_values, hm_nint count);
/* Gets an item by its index. Returns HM_ERROR_OUT_OF_RANGE if the index is out of range. The retrieved value
   is copied to a memory block pointed to by in_value, depending on item_size. */
hmError hmArrayGet(hmArray* array, hm_nint index, void* in_value);
/* See hmArrayGet. */
hmError hmArraySet(hmArray* array, hm_nint index, void* in_value);
/* Removes all the elements in the array, allowing to reuse it.
   Calls `item_dispose_func` on all the elements, if it's specified. */
hmError hmArrayClear(hmArray* array);
/* Gets access to the raw data of the array. Useful for fast iterations as it doesn't do range checks. */
#define hmArrayGetRaw(array, type) (type*)((array)->items)
#define hmArrayGetCount(array) (array)->count
/* Expands the array by initializing elements of the array with the given callback. If no callback is provided,
   initializes the elements with all zeros (useful only if the array contains only zeroable types: integers, floats, pointers).
   The user_data parameter is passed to the expand function as is (meaningful only if array_expand_func is provided). */
hmError hmArrayExpand(hmArray* array, hm_nint count, hmArrayExpandFunc array_expand_func, void* user_data);

#endif /* HM_ARRAY_H */
