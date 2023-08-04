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

#ifndef HM_UTF8_H
#define HM_UTF8_H

#include <core/common.h>

/* UTF8-related math expects chars to be unsigned, while our string's content is just `char*` for C interoperability,
   which is not guaranteed to be unsigned. So we define an explicitly signed UTF8 char type.
   See hmNextUTF8Rune(..) */
typedef unsigned char hm_utf8char;

/* Allows to iterate over runes in a UTF8 string (UTF8 is a variable-sized encoding so you can't just increment the index
   when iterating):
   `out_rune` is the next decoded rune.
   `out_offset` is the size of the decoded rune which tells the offset for the next rune.
   If the returned offset is equal to 0, iteration is over.

    The function is to be called in a loop:

        while ((err = hmNextUTF8Rune(content, length, &rune, &offset)) == HM_OK && offset > 0) {
            // Use the rune here.
            content += offset;
            length -= offset;
        }
*/
hmError hmNextUTF8Rune(const hm_utf8char* content, hm_nint length_in_bytes, hm_rune* out_rune, hm_nint* out_offset);

#endif /* HM_UTF8_H */
