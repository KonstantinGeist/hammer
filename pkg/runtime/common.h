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

#ifndef HM_RUNTIME_COMMON_H
#define HM_RUNTIME_COMMON_H

#include <core/common.h>

typedef hm_uint32 hm_metadata_id;
#define HM_MIN_METADATA_ID 0
#define HM_MAX_METADATA_ID HM_UINT32_MAX

typedef hm_uint16 hm_method_size;
#define HM_MIN_METHOD_SIZE 1 /* Methods of size 0 are meaningless. */
#define HM_MAX_METHOD_SIZE HM_UINT16_MAX /* Method size is restricted to 64KB similar to Java. */

#define hmMetadataIDHashFunc hmUint32HashFunc
#define hmMetadataIDEqualsFunc hmUint32EqualsFunc

#endif /* HM_RUNTIME_COMMON_H */
