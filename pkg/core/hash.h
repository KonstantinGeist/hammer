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

#ifndef HM_HASH_H
#define HM_HASH_H

#include <core/common.h>

/* Hashes a byte buffer by mixing it with a predefined salt (to fight against hash DoS attacks).
   The salt should be stable for the duration of the process (subprocess) but different across different
   process (subprocess) runs.
   Returns uint32 (not native int) to make hashing more predictable across platforms. Unsigned values
   also allow wraparounds without undefined behavior. */
hm_uint32 hmHash(void* bytes, hm_nint size, hm_uint32 salt);

#endif /* HM_HASH_H */
