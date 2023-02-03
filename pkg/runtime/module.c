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

#include <runtime/module.h>
#include <runtime/class.h>
#include <runtime/metadata.h>

hmError hmCreateModule(hmAllocator* allocator, hm_metadata_id module_id, hmString* name, hmModule* in_module)
{
    HM_TRY(hmValidateMetadataName(name));
    HM_TRY(hmStringDuplicate(allocator, name, &in_module->name));
    hmError err = hmCreateHashMap(
        allocator,
        &hmMetadataIDHashFunc,
        &hmMetadataIDEqualsFunc,
        HM_NULL,            /* key_dispose_func */
        HM_NULL,            /* value_dispose_func */
        sizeof(hm_metadata_id),
        sizeof(hmClass*),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        0,
        &in_module->classes
    );
    if (err != HM_OK) {
        err = hmMergeErrors(err, hmStringDispose(&in_module->name));
    }
    in_module->module_id = module_id;
    return HM_OK;
}

hmError hmModuleDispose(hmModule* module)
{
    hmError err = hmStringDispose(&module->name);
    return hmMergeErrors(err, hmHashMapDispose(&module->classes));
}

hmError hmModuleDisposeFunc(void* object)
{
    return hmModuleDispose((hmModule*)object);
}
