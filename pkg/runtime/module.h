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

#ifndef HM_MODULE_H
#define HM_MODULE_H

#include <core/common.h>
#include <core/string.h>
#include <collections/hashmap.h>
#include <runtime/common.h>

typedef struct {
    hmString       name;    /* The name of the module. Should be unique in a given module registry. */
    hmHashMap      classes; /* hmHashMap<hm_metadata_id, hmClass*> */
    hm_metadata_id module_id;
} hmModule;

hmError hmCreateModule(hmAllocator* allocator, hm_metadata_id module_id, hmString* name, hmModule* in_module);
hmError hmModuleDispose(hmModule* module);
hmError hmModuleDisposeFunc(void* object);
#define hmModuleGetName(module) &(module)->name
#define hmModuleGetID(module) (module)->module_id

#endif /* HM_MODULE_H */
