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

#ifndef HM_CLASS_H
#define HM_CLASS_H

#include <core/string.h>
#include <collections/hashmap.h>
#include <runtime/common.h>

typedef struct hmClass_ {
    hmString       name;    /* The name of the class (NOT fully qualified, for example: "StringBuilder"). The name
                               should be unique in a given module. */
    hmHashMap      methods; /* hmHashMap<hm_metadata_id, hmMethod*> */
    hm_metadata_id class_id;
} hmClass;

#define hmClassGetName(hm_class) (hm_class)->name
#define hmClassGetID(hm_class) (hm_class)->class_id

#endif /* HM_CLASS_H */
