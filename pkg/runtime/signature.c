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

#include <runtime/signature.h>

#define HM_SIGNATURE_BOOL_DESC        HM_CHAR('B')
#define HM_SIGNATURE_INT_DESC         HM_CHAR('I')
#define HM_SIGNATURE_FLOAT_DESC       HM_CHAR('F')
#define HM_SIGNATURE_VOID_DESC        HM_CHAR('V')
#define HM_SIGNATURE_CLASS_DESC_BEGIN HM_CHAR('{') /* A fully-qualified class name including the parent module,
                                                      for example: "{core.StringBuilder}" */
#define HM_SIGNATURE_CLASS_DESC_END   HM_CHAR('}')

/* TODO clean up */
hm_bool hmIsValidSignatureDesc(hmString* signature_desc)
{
    hm_nint length = hmStringGetLength(signature_desc);
    if (!length) {
        /* A signature must have at least a valid return type (as the first element). */
        return HM_FALSE;
    }
    hm_char* chars = hmStringGetChars(signature_desc);
    hm_bool is_class_desc_current = HM_FALSE;
    for (hm_nint i = 0; i < length; i++) {
        hm_char c = chars[i];
        if (c == HM_SIGNATURE_CLASS_DESC_BEGIN) {
            if (is_class_desc_current) {
                return HM_FALSE; /* Class desc opening tag was specified twice. */
            }
            is_class_desc_current = HM_TRUE;
            continue;
        }
        if (c == HM_SIGNATURE_CLASS_DESC_END) {
            if (!is_class_desc_current) {
                return HM_FALSE; /* Class desc closing tag was specified twice. */
            }
            is_class_desc_current = HM_FALSE;
            continue;
        }
        if (is_class_desc_current) { /* Skip everything between class desc tags. */
            continue;
        }
        if (c == HM_SIGNATURE_VOID_DESC) {
            /* "void" can only be specified as a return type (as the first element). */
            if (i > 0) {
                return HM_FALSE;
            }
            continue;
        }
        hm_bool is_primitive = (c == HM_SIGNATURE_BOOL_DESC)
                            || (c == HM_SIGNATURE_INT_DESC)
                            || (c == HM_SIGNATURE_FLOAT_DESC);
        if (!is_primitive) {
            return HM_FALSE;
        }
    }
    if (is_class_desc_current) {
        return HM_FALSE; /* Expecting a class desc closing tag. */
    }
    return HM_TRUE;
}
