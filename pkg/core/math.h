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

#ifndef HM_MATH_H
#define HM_MATH_H

#include <core/common.h>

/* A safe addition operation: returns HM_ERROR_OVERFLOW on overflow. */
hmError hmAddNint(hm_nint a, hm_nint b, hm_nint* result);

#endif /* HM_MATH_H */
