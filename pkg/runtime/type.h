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

#ifndef HM_TYPE_H
#define HM_TYPE_H

#include <core/common.h>

/* A type kind unites primitive types (integers, floats etc.) and classes. It is used to mark types of values in
   various metadata. */
typedef hm_uint8 hmTypeKind;
#define HM_TYPEKIND_VOID  ((hmType)0) /* Specifies that the value has no type assigned. Useful only for returned values. */
#define HM_TYPEKIND_INT32 ((hmType)1) /* A 32-bit integer. */
#define HM_TYPEKIND_CLASS ((hmType)2) /* A class type (for objects). */

/* A typeref is a reference to a type, which can be a primitive type or an object of a certain class. */
typedef struct {
    struct __hmClass*   class;     /* This value is non-null only if type_kind == HM_TYPEKIND_CLASS. */
    hmTypeKind          type_kind; /* Specifies the type kind of the value this typeref refers to. */
} hmTypeRef;

#endif /* HM_TYPE_H */
