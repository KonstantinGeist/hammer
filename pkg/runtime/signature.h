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

#include <collections/array.h>

typedef struct {
    struct hmClass_* return_class;
    hmArray          param_classes; /* hmArray<hmClass*> */
} hmSignature;

#endif /* HM_SIGNATURE_H */
