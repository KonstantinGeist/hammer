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

#include <core/utils.h>
#include <core/math.h>

#include <stdio.h>

/* Necessary for better alignment on typical CPU's for faster memory access. */
#define HM_ALLOC_SIZE_ALIGNMENT 16

hm_nint hmAlignSize(hm_nint sz)
{
    hm_nint sz_with_alignment = 0;
    hmError err = hmAddNint(sz, HM_ALLOC_SIZE_ALIGNMENT - 1, &sz_with_alignment);
    hmPanicIf(err != HM_OK, "overflow in hmAlignSize(..)"); /* mentioned in the function's docs */
    return sz_with_alignment & (-HM_ALLOC_SIZE_ALIGNMENT);
}

void hmLog(const char* msg)
{
    printf("%s\n", msg);
}

void hmPanicIf(hm_bool condition, const char* description)
{
    if (condition) {
        printf("panic: %s\n", description);
        exit(1);
    }
}
