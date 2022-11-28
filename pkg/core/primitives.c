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

#include <core/primitives.h>
#include <core/hash.h>

hm_uint32 hmNintHashFunc(void* key, hm_uint32 salt)
{
    return hmHash(key, sizeof(hm_nint), salt);
}

hm_bool hmNintEqualsFunc(void* value1, void* value2)
{
    hm_nint a = *((hm_nint*)(value1));
    hm_nint b = *((hm_nint*)(value2));
    return a == b;
}

hm_uint32 hmInt32HashFunc(void* key, hm_uint32 salt)
{
    return hmHash(key, sizeof(hm_int32), salt);
}

hm_bool hmInt32EqualsFunc(void* value1, void* value2)
{
    hm_int32 a = *((hm_int32*)(value1));
    hm_int32 b = *((hm_int32*)(value2));
    return a == b;
}
