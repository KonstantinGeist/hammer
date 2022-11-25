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

#include <core/hash.h>

/* Based on github.com/wangyi-fudan/wyhash which is in public domain. */

static hm_uint32 _wyr32(const hm_uint8 *p)
{
    hm_uint32 v;
    memcpy(&v, p, sizeof(hm_uint32));
    return v;
}

static hm_uint32 _wyr24(const hm_uint8 *p, hm_uint32 k)
{
    return (((hm_uint32)p[0]) << 16) | (((hm_uint32)p[k >> 1]) << 8) | p[k - 1];
}

static void _wymix32(hm_uint32 *A, hm_uint32 *B)
{
    uint64_t c = *A ^ 0x53c5ca59u;
    c *= *B ^ 0x74743c1bu;
    *A = (hm_uint32)c;
    *B = (hm_uint32)(c >> 32);
}

hm_uint32 hmHash(void* bytes, hm_nint size, hm_uint32 salt)
{
    if (!size) {
        return salt;
    }
    const hm_uint8 *p = (const hm_uint8*)bytes;
    hm_uint64 i = size;
    hm_uint32 see1 = (hm_uint32)size;
    salt ^= (hm_uint32)(size >> 32);
    _wymix32(&salt, &see1);
    for (; i > 8; i-= 8, p+=8) {
        salt^=_wyr32(p);
        see1^=_wyr32(p + 4);
        _wymix32(&salt, &see1);
    }
    if (i>=4) {
        salt^=_wyr32(p);
        see1^=_wyr32(p + i - 4);
     } else if (i) {
        salt^=_wyr24(p, (hm_uint32)i);
     }
    _wymix32(&salt, &see1);
    _wymix32(&salt, &see1);
    return salt ^ see1;
}
