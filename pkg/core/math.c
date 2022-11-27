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
