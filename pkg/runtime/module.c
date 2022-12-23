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
#include <core/allocator.h>
#include <core/primitives.h>
#include <runtime/image.h>

static hmError hmModuleRegistry_enumModulesFunc(hmModuleMetadata* metadata, void* user_data);
static hmError hmModuleRegistry_enumClassesFunc(hmClassMetadata* metadata, void* user_data);
static hmError hmModuleRegistry_enumMethodsFunc(hmMethodMetadata* metadata, void* user_data);
static hmError hmModuleDisposeFunc(void* object);
static hmError hmClassDisposeFunc(void* object);

hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry)
{
    HM_TRY(hmCreateHashMapWithStringKeys(
        allocator,
        &hmModuleDisposeFunc, /* value_dispose_func */
        sizeof(hmModule),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        0, /* hash map keys are not user-controlled, OK to leave seed as 0 here and below */
        &in_registry->name_to_module_map
    ));
    hmError err = hmCreateHashMap(
        allocator,
        &hmInt32HashFunc,
        &hmInt32EqualsFunc,
        HM_NULL,            /* key_dispose_func */
        HM_NULL,            /* value_dispose_func */
        sizeof(hm_int32),
        sizeof(hmModule*),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        0,
        &in_registry->module_id_to_module_ref_map
    );
    if (err != HM_OK) {
        return hmMergeErrors(err, hmHashMapDispose(&in_registry->name_to_module_map));
    }
    in_registry->allocator = allocator;
    return HM_OK;
}

hmError hmModuleRegistryDispose(hmModuleRegistry* registry)
{
    hmError err = hmHashMapDispose(&registry->name_to_module_map);
    return hmMergeErrors(err, hmHashMapDispose(&registry->module_id_to_module_ref_map));
}

hmError hmModuleRegistryLoadFromImage(hmModuleRegistry* registry, hmString* image_path)
{
    return hmEnumMetadataInImage(
        image_path,
        &hmModuleRegistry_enumModulesFunc,
        &hmModuleRegistry_enumClassesFunc,
        &hmModuleRegistry_enumMethodsFunc,
        registry
    );
}

hmError hmModuleRegistryGetModuleRefByName(hmModuleRegistry* registry, hmString* name, hmModule** out_module)
{
    void* module_ref;
    HM_TRY(hmHashMapGetRef(&registry->name_to_module_map, name, &module_ref));
    *out_module = (hmModule*)module_ref;
    return HM_OK;
}

hmError hmModuleGetClassRefByName(hmModule* module, hmString* name, hmClass** out_class)
{
    void* class_ref;
    HM_TRY(hmHashMapGetRef(&module->name_to_class_map, name, &class_ref));
    *out_class = (hmClass*)class_ref;
    return HM_OK;
}

static hmError hmModuleDispose(hmModule* module)
{
    hmError err = hmStringDispose(&module->name);
    err = hmMergeErrors(err, hmHashMapDispose(&module->name_to_class_map));
    return hmMergeErrors(err, hmHashMapDispose(&module->class_id_to_class_ref_map));
}

static hmError hmModuleDisposeFunc(void* object)
{
    hmModule* module = (hmModule*)object;
    return hmModuleDispose(module);
}

static hmError hmClassDispose(hmClass* hm_class)
{
    return hmStringDispose(&hm_class->name);
}

static hmError hmClassDisposeFunc(void* object)
{
    hmClass* hm_class = (hmClass*)object;
    return hmClassDispose(hm_class);
}

static hmError hmModuleRegistryValidateModuleDoesNotExist(hmModuleRegistry* registry, hm_int32 module_id, hmString* name)
{
    hm_bool found = hmHashMapContains(&registry->name_to_module_map, name);
    if (found) {
        return HM_ERROR_INVALID_IMAGE;
    }
    found = hmHashMapContains(&registry->module_id_to_module_ref_map, &module_id);
    if (found) {
        return HM_ERROR_INVALID_IMAGE;
    }
    return HM_OK;
}

static hmError hmCreateModule(hmAllocator* allocator, hm_int32 module_id, hmString* name, hmModule* in_module)
{
    HM_TRY(hmStringDuplicate(allocator, name, &in_module->name));
    hmError err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmClassDisposeFunc, /* value_dispose_func */
        sizeof(hmClass),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        0, /* hash map keys are not user-controlled, OK to leave seed as 0 here and below */
        &in_module->name_to_class_map
    );
    if (err != HM_OK) {
        return hmMergeErrors(err, hmStringDispose(&in_module->name));
    }
    err = hmCreateHashMap(
        allocator,
        &hmInt32HashFunc,
        &hmInt32EqualsFunc,
        HM_NULL,            /* key_dispose_func */
        HM_NULL,            /* value_dispose_func */
        sizeof(hm_int32),
        sizeof(hmClass*),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        0,
        &in_module->class_id_to_class_ref_map
    );
    if (err != HM_OK) {
        err = hmMergeErrors(err, hmStringDispose(&in_module->name));
        return hmMergeErrors(err, hmHashMapDispose(&in_module->name_to_class_map));
    }
    in_module->module_id = module_id;
    return HM_OK;
}

/* Ownership of `name`, `module` is transferred to this function. */
static hmError hmModuleRegistryStoreModule(hmModuleRegistry* registry, hm_int32 module_id, hmString* name, hmModule* module)
{
    hmError err = HM_OK;
    hm_bool name_and_module_are_saved_to_map = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmHashMapPut(&registry->name_to_module_map, name, module));
    name_and_module_are_saved_to_map = HM_FALSE;
    void* module_ref;
    HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&registry->name_to_module_map, name, &module_ref));
    err = hmHashMapPut(&registry->module_id_to_module_ref_map, &module_id, &module_ref);
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (name_and_module_are_saved_to_map) {
            err = hmMergeErrors(err, hmStringDispose(name));
            err = hmMergeErrors(err, hmModuleDispose(module));
        } else {
            err = hmMergeErrors(err, hmHashMapRemove(&registry->name_to_module_map, name, HM_NULL));
            err = hmMergeErrors(err, hmHashMapRemove(&registry->module_id_to_module_ref_map, &module_id, HM_NULL));
        }
    }
    return err;
}

static hmError hmModuleRegistryGetModuleRefByID(hmModuleRegistry* registry, hm_int32 module_id, hmModule** out_module)
{
    void* module_ref;
    HM_TRY(hmHashMapGet(&registry->module_id_to_module_ref_map, &module_id, &module_ref));
    *out_module = (hmModule*)module_ref;
    return HM_OK;
}

static hmError hmModuleRegistry_enumModulesFunc(hmModuleMetadata* metadata, void* user_data)
{
    hmModuleRegistry* registry = (hmModuleRegistry*)user_data;
    HM_TRY(hmModuleRegistryValidateModuleDoesNotExist(registry, metadata->module_id, &metadata->name));
    hmModule module;
    HM_TRY(hmCreateModule(registry->allocator, metadata->module_id, &metadata->name, &module));
    hmString name;
    hmError err = hmStringDuplicate(registry->allocator, &metadata->name, &name);
    if (err != HM_OK) {
        return hmMergeErrors(err, hmModuleDispose(&module));
    }
    /* module, name are moved to hmModuleRegistryStoreModule(..) */
    HM_TRY(hmModuleRegistryStoreModule(registry, metadata->module_id, &name, &module));
    return HM_OK;
}

static hmError hmModuleValidateClassDoesNotExist(hmModule* module, hm_int32 class_id, hmString* name)
{
    hm_bool found = hmHashMapContains(&module->name_to_class_map, name);
    if (found) {
        return HM_ERROR_INVALID_IMAGE;
    }
    found = hmHashMapContains(&module->class_id_to_class_ref_map, &class_id);
    if (found) {
        return HM_ERROR_INVALID_IMAGE;
    }
    return HM_OK;
}

static hmError hmCreateClass(hmAllocator* allocator, hm_int32 class_id, hmString* name, hmClass* in_class)
{
    HM_TRY(hmStringDuplicate(allocator, name, &in_class->name));
    in_class->class_id = class_id;
    return HM_OK;
}

/* name, hm_class are owned by this function */
static hmError hmModuleStoreClass(hmModule* module, hm_int32 class_id, hmString* name, hmClass* hm_class)
{
    hmError err = HM_OK;
    hm_bool name_and_class_owned = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmHashMapPut(&module->name_to_class_map, name, hm_class));
    name_and_class_owned = HM_FALSE;
    void* class_ref;
    HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&module->name_to_class_map, name, &class_ref));
    err = hmHashMapPut(&module->class_id_to_class_ref_map, &class_id, (hmClass*)class_ref);
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (name_and_class_owned) {
            err = hmMergeErrors(err, hmStringDispose(name));
            err = hmMergeErrors(err, hmClassDispose(hm_class));
        } else {
            err = hmMergeErrors(err, hmHashMapRemove(&module->name_to_class_map, name, HM_NULL));
            err = hmMergeErrors(err, hmHashMapRemove(&module->class_id_to_class_ref_map, &class_id, HM_NULL));
        }
    }
    return err;
}

static hmError hmModuleRegistry_enumClassesFunc(hmClassMetadata* metadata, void* user_data)
{
    hmModuleRegistry* registry = (hmModuleRegistry*)user_data;
    hmModule* module_ref;
    hmError err = hmModuleRegistryGetModuleRefByID(registry, metadata->module_id, &module_ref);
    if (err != HM_OK) {
        return HM_ERROR_INVALID_IMAGE;
    }
    HM_TRY(hmModuleValidateClassDoesNotExist(module_ref, metadata->class_id, &metadata->name));
    hmClass hm_class;
    HM_TRY(hmCreateClass(registry->allocator, metadata->class_id, &metadata->name, &hm_class));
    hmString name;
    err = hmStringDuplicate(registry->allocator, &metadata->name, &name);
    if (err != HM_OK) {
        return hmMergeErrors(err, hmClassDispose(&hm_class));
    }
    /* hm_class, name are moved to hmModuleStoreClass(..) */
    HM_TRY(hmModuleStoreClass(module_ref, metadata->class_id, &name, &hm_class));
    return HM_OK;
}

// TODO
#include <stdio.h>

static hmError hmModuleRegistry_enumMethodsFunc(hmMethodMetadata* metadata, void* user_data)
{
    // TODO
    printf(
        "method: method_id=%d class_id=%d name=%s signature=%s blob_size=%d\n",
        (int)metadata->method_id,
        (int)metadata->class_id,
        hmStringGetRaw(&metadata->name),
        hmStringGetRaw(&metadata->signature),
        (int)metadata->body.size
    );
    return HM_OK;
}
