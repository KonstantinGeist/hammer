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

#include <stdio.h>  /* for printf(..) */
#include <stdlib.h> /* for exit(..) */

hm_nint hmAlignSize(hm_nint size)
{
    hm_nint size_with_alignment = 0;
    hmError err = hmAddNint(size, HM_ALLOC_SIZE_ALIGNMENT - 1, &size_with_alignment);
    hmPanicIf(err != HM_OK, "overflow in hmAlignSize(..)"); /* mentioned in the function's docs */
    return size_with_alignment & (-HM_ALLOC_SIZE_ALIGNMENT);
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
