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

#include <core/math.h>

hmError hmAddNint(hm_nint a, hm_nint b, hm_nint* result)
{
    if (b > HM_NINT_MAX - a) {
        return HM_ERROR_OVERFLOW;
    }
    *result = a + b;
    return HM_OK;
}

hmError hmMulNint(hm_nint a, hm_nint b, hm_nint* result)
{
    if (a == 0 || b == 0) {
        *result = 0;
        return HM_OK;
    }
    hm_nint mul_result = a * b; /* hm_nint is unsigned, so it's OK to wrap around here (no undefined behavior) */
    if (a != mul_result / b) {
        return HM_ERROR_OVERFLOW;
    }
    *result = mul_result;
    return HM_OK;
}

hmError hmAddMulNint(hm_nint a, hm_nint b, hm_nint c, hm_nint* result)
{
    hm_nint mul_result = 0;
    HM_TRY(hmMulNint(b, c, &mul_result));
    return hmAddNint(a, mul_result, result);
}

/* See hmAddNint(..) */
hmError hmAddMillis(hm_millis a, hm_millis b, hm_millis* result)
{
    if (b > HM_MILLIS_MAX - a) {
        return HM_ERROR_OVERFLOW;
    }
    *result = a + b;
    return HM_OK;
}
