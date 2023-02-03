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

#include <runtime/moduleregistry.h>
#include <runtime/module.h>

static hmError hmModuleRegistry_enumModulesFunc(hmModuleMetadata* metadata, void* user_data);
static hmError hmModuleRegistry_enumClassesFunc(hmClassMetadata* metadata, void* user_data);
static hmError hmModuleRegistry_enumMethodsFunc(hmMethodMetadata* metadata, void* user_data);

hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry)
{
    HM_TRY(hmCreateHashMap(
        allocator,
        &hmMetadataIDHashFunc,
        &hmMetadataIDEqualsFunc,
        HM_NULL,            /* key_dispose_func */
        HM_NULL,            /* value_dispose_func */
        sizeof(hm_metadata_id),
        sizeof(hmModule*),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        0,
        &in_registry->modules
    ));
    in_registry->allocator = allocator;
    return HM_OK;
}

hmError hmModuleRegistryDispose(hmModuleRegistry* registry)
{
    return hmHashMapDispose(&registry->modules);
}

hmError hmModuleRegistryLoad(hmModuleRegistry* registry, hmMetadataLoader* metadata_loader)
{
    HM_TRY(hmMetadataLoaderEnumMetadata(
        metadata_loader,
        &hmModuleRegistry_enumModulesFunc,
        &hmModuleRegistry_enumClassesFunc,
        &hmModuleRegistry_enumMethodsFunc,
        registry
    ));
    return HM_OK;
}

static hmError hmModuleRegistry_enumModulesFunc(hmModuleMetadata* metadata, void* user_data)
{
    return HM_OK;
}

static hmError hmModuleRegistry_enumClassesFunc(hmClassMetadata* metadata, void* user_data)
{
    return HM_OK;
}

static hmError hmModuleRegistry_enumMethodsFunc(hmMethodMetadata* metadata, void* user_data)
{
    return HM_OK;
}
