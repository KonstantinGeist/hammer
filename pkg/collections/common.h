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

#ifndef HM_COLLECTIONS_COMMON_H
#define HM_COLLECTIONS_COMMON_H

#include <core/common.h>

typedef hm_uint8 hmComparisonResult;
#define HM_COMPARISON_RESULT_LESS ((hmComparisonResult)-1)
#define HM_COMPARISON_RESULT_EQUAL ((hmComparisonResult)0)
#define HM_COMPARISON_RESULT_GREATER ((hmComparisonResult)1)

typedef hmComparisonResult (*hmCompareFunc)(void* item1, void* item2, void* user_data);

#endif /* HM_COLLECTIONS_COMMON_H */
