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

#ifndef HM_TEST_H
#define HM_TEST_H

#include <assert.h>
#include "../src/common.h"

#define HM_TEST_ASSERT(expr) assert(expr)
#define HM_TEST_ASSERT_OK(err) assert((err) == HM_OK)

#endif /* HM_TEST_H */
