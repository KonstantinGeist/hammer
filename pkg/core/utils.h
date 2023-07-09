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

#ifndef HM_UTILS_H
#define HM_UTILS_H

#include <core/common.h>

#include <string.h> /* for memcpy(..), memset(..) and memcmp(..) */

/* Necessary for better alignment on typical CPU's for faster memory access. */
#define HM_ALLOC_SIZE_ALIGNMENT 16

/* Aligns the size up to the value most suited for Hammer's allocators.
   WARNING Checks for overflow must be made before calling this function,
   because it panics on overflow. */
hm_nint hmAlignSize(hm_nint size);
/* Useful for logging when there's no other way to report an error. */
void hmLog(const char* msg);
void hmPanicIf(hm_bool condition, const char* description);

/* We use own wrappers to be able to swap them with some instrumentation and safer variants later. */
#define hmCopyMemory(dest, src, size) memcpy(dest, src, size)
#define hmCompareMemory(value1, value2, size) memcmp(value1, value2, size)
#define hmZeroMemory(dest, size) memset(dest, 0, size)

#endif /* HM_UTILS_H */
