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

#ifndef HM_SIGNATURE_H
#define HM_SIGNATURE_H

#include <core/common.h>
#include <core/string.h>

/* Verifies that the signature is valid syntactically.
   It's a quick verification for faster loading (without trying to resolve class references or validate class names);
   full verification is done separately during method body verification. */
hm_bool hmIsValidSignatureDesc(hmString* signature_desc);

#endif /* HM_SIGNATURE_H */
