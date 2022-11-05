// *****************************************************************************
//
//  Copyright (c) Konstantin Geist. All rights reserved.
//
//  The use and distribution terms for this software are contained in the file
//  named License.txt, which can be found in the root of this distribution.
//  By using this software in any fashion, you are agreeing to be bound by the
//  terms of this license.
//
//  You must not remove this notice, or any other, from this software.
//
// *****************************************************************************

#ifndef HM_PRIMITIVES_H
#define HM_PRIMITIVES_H

#include <core/common.h>

hm_uint32 hmNintHashFunc(void* key);
hm_bool hmNintEqualsFunc(void* value1, void* value2);
hm_uint32 hmInt32HashFunc(void* key);
hm_bool hmInt32EqualsFunc(void* value1, void* value2);

#endif /* HM_PRIMITIVES_H */
