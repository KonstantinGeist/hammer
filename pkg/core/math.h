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

#ifndef HM_MATH_H
#define HM_MATH_H

#include <core/common.h>

/* A safe addition operation: returns HM_ERROR_OVERFLOW on overflow. */
hmError hmAddNint(hm_nint a, hm_nint b, hm_nint* result);
/* Same as hmAddNint(..), but accepts 3 arguments instead (for terser code). */
hmError hmAddNint3(hm_nint a, hm_nint b, hm_nint c, hm_nint* result);
/* A safe multiplication operation: returns HM_ERROR_OVERFLOW on overflow. */
hmError hmMulNint(hm_nint a, hm_nint b, hm_nint* result);
/* A safe addition+multiplication operation: returns HM_ERROR_OVERFLOW on overflow.
   Useful for calculating item addresses inside an array: base + index * size */
hmError hmAddMulNint(hm_nint a, hm_nint b, hm_nint c, hm_nint* result);
/* See hmAddNint(..) */
hmError hmAddMillis(hm_millis a, hm_millis b, hm_millis* result);
/* Takes the absolute value of a 32-bit integer. Since taking the absolute value of the most negative integer
   is not defined, this function is designed to return HM_ERROR_INVALID_ARGUMENT if the value is HM_INT32_MIN. */
hmError hmAbsInt32(hm_int32 value, hm_int32* out_value);

#endif /* HM_MATH_H */
