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

#include <stdlib.h> /* for qsort_r(..) */

typedef struct {
    void*         user_data;
    hmCompareFunc compare_func;
} hmArraySortContext;

static int hmAdaptGlibcSortFuncToHammer(const void* value1, const void* value2, void* arg)
{
    hmArraySortContext* context = (hmArraySortContext*)arg;
    hmComparisonResult result = context->compare_func((void*)value1, (void*)value2, context->user_data);
    switch (result) {
        case HM_COMPARISON_RESULT_LESS:
            return -1;
        case HM_COMPARISON_RESULT_EQUAL:
            return 0;
        case HM_COMPARISON_RESULT_GREATER:
        default:
            return 1;
    }
}

/* Using glibc's non-standard qsort_r(..) function as it's much more optimized than we could ever do it ourselves. */
hmError hmArraySort(hmArray* array, hmCompareFunc compare_func, void* user_data)
{
    if (array->count < 2) {
        return HM_OK;
    }
    hmArraySortContext context;
    context.user_data = user_data;
    context.compare_func = compare_func;
    qsort_r(array->items, array->count, array->item_size, &hmAdaptGlibcSortFuncToHammer, &context);
    return HM_OK;
}
