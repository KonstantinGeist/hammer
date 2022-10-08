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

#include "primitives.h"

int hmNintHashFunc(void* key)
{
    int x = (int)(*((hm_nint*)key));
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = (x >> 16) ^ x;
    return x;
}

hm_bool hmNintEqualsFunc(void* value1, void* value2)
{
    hm_nint a = *((hm_nint*)(value1));
    hm_nint b = *((hm_nint*)(value2));
    return a == b;
}
