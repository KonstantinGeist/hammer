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

#ifndef HM_METHOD_H
#define HM_METHOD_H

#include <core/common.h>
#include <core/string.h>
#include <runtime/common.h>
#include <runtime/signature.h>

typedef struct {
    hm_uint8*      opcodes;
    hm_method_size size;
} hmMethodBody;

typedef struct {
    hmString       name;      /* The name of the method which should be unique in a given class. */
    hmSignature    signature; /* Describes the parameters and the return type. */
    hmMethodBody   hl_body;   /* High-level bytecode as stored in metadata. */
    hm_metadata_id method_id;
} hmMethod;

#define hmMethodGetName(method) (method)->name
#define hmMethodGetID(method) (method)->method_id

#endif /* HM_METHOD_H */
