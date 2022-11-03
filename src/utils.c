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

#include "utils.h"

// Necessary for better alignment on typical CPU's for faster memory access.
#define HM_ALLOC_SIZE_ALIGNMENT 16

hm_nint hmAlignSize(hm_nint sz)
{
    return (sz + HM_ALLOC_SIZE_ALIGNMENT - 1) & (-HM_ALLOC_SIZE_ALIGNMENT);
}
