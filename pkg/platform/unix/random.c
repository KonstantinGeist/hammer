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

#include <core/environment.h>

#include <sys/random.h>

hm_int32 hmGenerateSeed()
{
    hm_int32 ret_value = 0;
    ssize_t result = getrandom(&ret_value, sizeof(ret_value), 0);
    if (result == -1 || ret_value == 0) { /* fall back on the current time if /dev/urandom is not available */
        ret_value = hmGetTickCount();
    }
    return ret_value;
}
