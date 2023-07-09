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

#include <core/string.h>

#include <string.h> /* for strcoll(..) */

/* Assumes the current locale is UTF8, which is the standard in modern Unix systems. */
hmComparisonResult hmStringCompare(hmString* string1, hmString* string2)
{
    int result = strcoll(hmStringGetCString(string1), hmStringGetCString(string2));
    if (result < 0) {
        return HM_COMPARISON_RESULT_LESS;
    } else if (result > 0) {
        return HM_COMPARISON_RESULT_GREATER;
    } else {
        return HM_COMPARISON_RESULT_EQUAL;
    }
}
