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
#include <core/utils.h>
#include <runtime/metadata.h>
#include <runtime/signature.h>

static hmError hmModuleRegistry_enumModulesFunc(hmModuleMetadata* metadata, void* user_data);
static hmError hmModuleRegistry_enumClassesFunc(hmClassMetadata* metadata, void* user_data);
static hmError hmModuleRegistry_enumMethodsFunc(hmMethodMetadata* metadata, void* user_data);
static hmError hmModuleDisposeFunc(void* object);
static hmError hmClassDisposeFunc(void* object);
static hmError hmMethodDisposeFunc(void* object);
static hmError hmModuleRegistryGetModuleRefByID(hmModuleRegistry* registry, hm_metadata_id module_id, hmModule** out_module);
static hmError hmModuleResolve(hmModule* module, hmModuleRegistry* module_registry);

hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry)
{
    HM_TRY(hmCreateHashMapWithStringKeys(
        allocator,
        &hmModuleDisposeFunc, /* value_dispose_func */
        sizeof(hmModule),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        0,                    /* hash map keys are not user-controlled, OK to leave seed as 0 here and below */
        &in_registry->name_to_module_map
    ));
    hmError err = HM_OK;
    hm_bool is_module_id_to_module_ref_map_initialized = HM_FALSE,
            is_modules_to_resolve_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmCreateHashMap(
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
        &in_registry->module_id_to_module_ref_map
    ));
    is_module_id_to_module_ref_map_initialized = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmCreateArray(
        allocator,
        sizeof(hmModule*),
        HM_ARRAY_DEFAULT_CAPACITY,
        HM_NULL,
        &in_registry->module_ids_to_resolve
    ));
    is_modules_to_resolve_initialized = HM_TRUE;
    in_registry->allocator = allocator;
HM_ON_FINALIZE
    if (err != HM_OK) {
        err = hmMergeErrors(err, hmHashMapDispose(&in_registry->name_to_module_map));
        if (is_module_id_to_module_ref_map_initialized) {
            err = hmMergeErrors(err, hmHashMapDispose(&in_registry->module_id_to_module_ref_map));
        }
        if (is_modules_to_resolve_initialized) {
            err = hmMergeErrors(err, hmArrayDispose(&in_registry->module_ids_to_resolve));
        }
    }
    return HM_OK;
}

hmError hmModuleRegistryDispose(hmModuleRegistry* registry)
{
    hmError err = hmHashMapDispose(&registry->name_to_module_map);
    err = hmMergeErrors(err, hmHashMapDispose(&registry->module_id_to_module_ref_map));
    return hmMergeErrors(err, hmArrayDispose(&registry->module_ids_to_resolve));
}

hmError hmModuleRegistryLoad(hmModuleRegistry* registry, hmMetadataLoader* metadata_loader)
{
    /* First step: load modules. */
    HM_TRY(hmMetadataLoaderEnumMetadata(
        metadata_loader,
        &hmModuleRegistry_enumModulesFunc, /* populates `module_ids_to_resolve` */
        &hmModuleRegistry_enumClassesFunc,
        &hmModuleRegistry_enumMethodsFunc,
        registry
    ));
    /* Second step: resolve modules. */
    hmError err = HM_OK;
    hm_nint module_count_to_resolve = hmArrayGetCount(&registry->module_ids_to_resolve);
    hm_metadata_id* module_ids_to_resolve = hmArrayGetRaw(&registry->module_ids_to_resolve, hm_metadata_id);
    for (hm_nint i = 0; i < module_count_to_resolve; i++) {
        hm_metadata_id module_id = module_ids_to_resolve[i];
        hmModule* module_ref_to_resolve;
        HM_TRY_OR_FINALIZE(err, hmModuleRegistryGetModuleRefByID(registry, module_id, &module_ref_to_resolve));
        HM_TRY_OR_FINALIZE(err, hmModuleResolve(module_ref_to_resolve, registry));
    }
    /* Third step: clean up. */
HM_ON_FINALIZE
    /* Note: we want our metadata loading to be atomic, i.e. either everything is loaded correctly, or nothing at all.
       We don't want to get a module which is half-resolved. Hence, on error, we remove all modules we added previously. */
    if (err != HM_OK) {
        for (hm_nint i = 0; i < module_count_to_resolve; i++) {
            hm_metadata_id module_id = module_ids_to_resolve[i];
            hmModule* module_ref_to_resolve;
            hmError module_ref_err = hmModuleRegistryGetModuleRefByID(registry, module_id, &module_ref_to_resolve);
            if (module_ref_err != HM_OK) { /* should never happen, just in case */
                continue;
            }
            err = hmMergeErrors(err, hmHashMapRemove(&registry->module_id_to_module_ref_map, &module_id, HM_NULL));
            err = hmMergeErrors(err, hmHashMapRemove(&registry->name_to_module_map, &module_ref_to_resolve->name, HM_NULL));
        }
    }
    return hmMergeErrors(err, hmArrayClear(&registry->module_ids_to_resolve));
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

hmError hmClassGetMethodRefByName(hmClass* hm_class, hmString* name, hmMethod** out_method)
{
    void* method_ref;
    HM_TRY(hmHashMapGetRef(&hm_class->name_to_method_map, name, &method_ref));
    *out_method = (hmMethod*)method_ref;
    return HM_OK;
}

hm_bool hmIsValidMetadataName(hmString* name)
{
    hm_nint length = hmStringGetLength(name);
    if (!length) {
        return HM_FALSE;
    }
    hm_char* chars = hmStringGetChars(name);
    for (hm_nint i = 0; i < length; i++) {
        hm_char c = chars[i];
        hm_bool is_digit = (c >= '0' && c <= '9');
        hm_bool is_valid = (c >= 'a' && c <= 'z')
                        || (c >= 'A' && c <= 'Z')
                        || is_digit
                        || c == '_';
        if (!is_valid) {
            return HM_FALSE;
        }
        if (is_digit && i == 0) {
            return HM_FALSE;
        }
    }
    return HM_TRUE;
}

static hm_bool hmValidateMetadataName(hmString* name)
{
    hm_bool is_valid = hmIsValidMetadataName(name);
    if (!is_valid) {
        return HM_ERROR_INVALID_DATA;
    }
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
    return hmModuleDispose((hmModule*)object);
}

static hmError hmModuleRegistryValidateModuleDoesNotExist(hmModuleRegistry* registry, hm_metadata_id module_id, hmString* name)
{
    hm_bool found = hmHashMapContains(&registry->name_to_module_map, name);
    if (found) {
        return HM_ERROR_INVALID_DATA;
    }
    found = hmHashMapContains(&registry->module_id_to_module_ref_map, &module_id);
    if (found) {
        return HM_ERROR_INVALID_DATA;
    }
    return HM_OK;
}

static hmError hmCreateModule(hmAllocator* allocator, hm_metadata_id module_id, hmString* name, hmModule* in_module)
{
    HM_TRY(hmValidateMetadataName(name));
    HM_TRY(hmStringDuplicate(allocator, name, &in_module->name));
    hmError err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmClassDisposeFunc, /* value_dispose_func */
        sizeof(hmClass),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        0, /* hash map keys are not user-controlled, OK to leave seed as 0 here and below */
        &in_module->name_to_class_map
    );
    if (err != HM_OK) {
        return hmMergeErrors(err, hmStringDispose(&in_module->name));
    }
    err = hmCreateHashMap(
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
static hmError hmModuleRegistryStoreModule(hmModuleRegistry* registry, hm_metadata_id module_id, hmString* name, hmModule* module)
{
    hmError err = HM_OK;
    hm_bool name_and_module_are_saved_to_map = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmHashMapPut(&registry->name_to_module_map, name, module));
    name_and_module_are_saved_to_map = HM_TRUE;
    void* module_ref;
    HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&registry->name_to_module_map, name, &module_ref));
    HM_TRY_OR_FINALIZE(err, hmHashMapPut(&registry->module_id_to_module_ref_map, &module_id, &module_ref));
    err = hmArrayAdd(&registry->module_ids_to_resolve, &module->module_id);
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (name_and_module_are_saved_to_map) {
            err = hmMergeErrors(err, hmHashMapRemove(&registry->module_id_to_module_ref_map, &module_id, HM_NULL));
            err = hmMergeErrors(err, hmHashMapRemove(&registry->name_to_module_map, name, HM_NULL));
        } else {
            err = hmMergeErrors(err, hmStringDispose(name));
            err = hmMergeErrors(err, hmModuleDispose(module));
        }
    }
    return err;
}

static hmError hmModuleRegistryGetModuleRefByID(hmModuleRegistry* registry, hm_metadata_id module_id, hmModule** out_module)
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
    return hmModuleRegistryStoreModule(registry, metadata->module_id, &name, &module);
}

static hmError hmModuleValidateClassDoesNotExist(hmModule* module, hm_metadata_id class_id, hmString* name)
{
    hm_bool found = hmHashMapContains(&module->name_to_class_map, name);
    if (found) {
        return HM_ERROR_INVALID_DATA;
    }
    found = hmHashMapContains(&module->class_id_to_class_ref_map, &class_id);
    if (found) {
        return HM_ERROR_INVALID_DATA;
    }
    return HM_OK;
}

static hmError hmCreateClass(hmAllocator* allocator, hm_metadata_id class_id, hmString* name, hmClass* in_class)
{
    HM_TRY(hmValidateMetadataName(name));
    HM_TRY(hmStringDuplicate(allocator, name, &in_class->name));
    hmError err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmMethodDisposeFunc, /* value_dispose_func */
        sizeof(hmMethod),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        0, /* hash map keys are not user-controlled, OK to leave seed as 0 here and below */
        &in_class->name_to_method_map
    );
    if (err != HM_OK) {
        return hmMergeErrors(err, hmStringDispose(&in_class->name));
    }
    err = hmCreateHashMap(
        allocator,
        &hmMetadataIDHashFunc,
        &hmMetadataIDEqualsFunc,
        HM_NULL,            /* key_dispose_func */
        HM_NULL,            /* value_dispose_func */
        sizeof(hm_metadata_id),
        sizeof(hmMethod*),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        0,
        &in_class->method_id_to_method_ref_map
    );
    if (err != HM_OK) {
        err = hmMergeErrors(err, hmStringDispose(&in_class->name));
        return hmMergeErrors(err, hmHashMapDispose(&in_class->name_to_method_map));
    }
    in_class->class_id = class_id;
    return HM_OK;
}

static hmError hmClassDispose(hmClass* hm_class)
{
    hmError err = hmStringDispose(&hm_class->name);
    err = hmMergeErrors(err, hmHashMapDispose(&hm_class->name_to_method_map));
    return hmMergeErrors(err, hmHashMapDispose(&hm_class->method_id_to_method_ref_map));
}

static hmError hmClassDisposeFunc(void* object)
{
    return hmClassDispose((hmClass*)object);
}

/* name, hm_class are owned by this function */
static hmError hmModuleStoreClass(hmModule* module, hm_metadata_id class_id, hmString* name, hmClass* hm_class)
{
    hmError err = HM_OK;
    hm_bool name_and_class_saved_to_map = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmHashMapPut(&module->name_to_class_map, name, hm_class));
    name_and_class_saved_to_map = HM_TRUE;
    void* class_ref;
    HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&module->name_to_class_map, name, &class_ref));
    err = hmHashMapPut(&module->class_id_to_class_ref_map, &class_id, &class_ref);
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (name_and_class_saved_to_map) {
            err = hmMergeErrors(err, hmHashMapRemove(&module->name_to_class_map, name, HM_NULL));
            err = hmMergeErrors(err, hmHashMapRemove(&module->class_id_to_class_ref_map, &class_id, HM_NULL));
        } else {
            err = hmMergeErrors(err, hmStringDispose(name));
            err = hmMergeErrors(err, hmClassDispose(hm_class));
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
        return HM_ERROR_INVALID_DATA;
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
    return hmModuleStoreClass(module_ref, metadata->class_id, &name, &hm_class);
}

static hmError hmValidateSignature(hmString* signature) /* TDOO refactor */
{
    hm_bool is_valid = hmIsValidSignatureDesc(signature);
    if (!is_valid) {
        return HM_ERROR_INVALID_DATA;
    }
    return HM_OK;
}

static hmError hmCreateSignature(hmAllocator* allocator, hmString* desc, hmSignature* in_signature)
{
    HM_TRY(hmValidateSignature(desc));
    HM_TRY(hmCreateArray(allocator, sizeof(hmClass*), HM_ARRAY_DEFAULT_CAPACITY, HM_NULL, &in_signature->param_classes));
    hmError err = hmStringDuplicate(allocator, desc, &in_signature->desc);
    if (err != HM_OK) {
        return hmMergeErrors(err, hmArrayDispose(&in_signature->param_classes));
    }
    in_signature->return_class = HM_NULL;
    in_signature->is_resolved = HM_FALSE;
    return HM_OK;
}

static hmError hmSignatureDispose(hmSignature* signature)
{
    hmError err = hmArrayDispose(&signature->param_classes);
    return hmMergeErrors(err, hmStringDispose(&signature->desc));
}

static hmError hmCreateMethod(
    hmAllocator* allocator,
    hm_metadata_id method_id,
    hmString* name,
    hmString* signature,
    const hm_uint8* opcodes,
    hm_method_size size,
    hmMethod* in_method
)
{
    HM_TRY(hmValidateMetadataName(name));
    hm_uint8* opcodes_copy = hmAlloc(allocator, sizeof(hm_uint8) * size);
    if (!opcodes_copy) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    hm_bool is_name_initialized = HM_FALSE,
            is_signature_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmStringDuplicate(allocator, name, &in_method->name));
    is_name_initialized = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmCreateSignature(allocator, signature, &in_method->signature));
    is_signature_initialized = HM_TRUE;
    /* No overflow-safe math because it must be below the limit if `opcodes` was successfully allocated before. */
    hmCopyMemory(opcodes_copy, opcodes, sizeof(hm_uint8) * size);
    in_method->allocator = allocator;
    in_method->method_id = method_id;
    in_method->hl_body.opcodes = opcodes_copy;
    in_method->hl_body.size = size;
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (is_name_initialized) {
            err = hmMergeErrors(err, hmStringDispose(&in_method->name));
        }
        if (is_signature_initialized) {
            err = hmMergeErrors(err, hmSignatureDispose(&in_method->signature));
        }
        hmFree(allocator, opcodes_copy);
    }
    return err;
}

static hmError hmMethodDispose(hmMethod* method)
{
    hmFree(method->allocator, method->hl_body.opcodes);
    hmError err = hmStringDispose(&method->name);
    return hmMergeErrors(err, hmSignatureDispose(&method->signature));
}

static hmError hmMethodDisposeFunc(void* object)
{
    return hmMethodDispose((hmMethod*)object);
}

static hmError hmModuleRegistryGetClassRefByModuleAndClassID(
    hmModuleRegistry* registry,
    hm_metadata_id module_id,
    hm_metadata_id class_id,
    hmClass** out_class
)
{
    void *module_ref, *class_ref;
    HM_TRY(hmHashMapGet(&registry->module_id_to_module_ref_map, &module_id, &module_ref));
    HM_TRY(hmHashMapGet(&((hmModule*)module_ref)->class_id_to_class_ref_map, &class_id, &class_ref));
    *out_class = (hmClass*)class_ref;
    return HM_OK;
}

static hmError hmModuleValidateMethodDoesNotExist(hmClass* hm_class, hm_metadata_id method_id, hmString* name)
{
    hm_bool found = hmHashMapContains(&hm_class->name_to_method_map, name);
    if (found) {
        return HM_ERROR_INVALID_DATA;
    }
    found = hmHashMapContains(&hm_class->method_id_to_method_ref_map, &method_id);
    if (found) {
        return HM_ERROR_INVALID_DATA;
    }
    return HM_OK;
}

/* name, method are owned by this function */
static hmError hmModuleStoreMethod(hmClass* hm_class, hm_metadata_id method_id, hmString* name, hmMethod* method)
{
    hmError err = HM_OK;
    hm_bool name_and_method_owned = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmHashMapPut(&hm_class->name_to_method_map, name, method));
    name_and_method_owned = HM_FALSE;
    void* method_ref;
    HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&hm_class->name_to_method_map, name, &method_ref));
    err = hmHashMapPut(&hm_class->method_id_to_method_ref_map, &method_id, &method_ref);
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (name_and_method_owned) {
            err = hmMergeErrors(err, hmStringDispose(name));
            err = hmMergeErrors(err, hmMethodDispose(method));
        } else {
            err = hmMergeErrors(err, hmHashMapRemove(&hm_class->name_to_method_map, name, HM_NULL));
            err = hmMergeErrors(err, hmHashMapRemove(&hm_class->method_id_to_method_ref_map, &method_id, HM_NULL));
        }
    }
    return err;
}

static hmError hmModuleRegistry_enumMethodsFunc(hmMethodMetadata* metadata, void* user_data)
{
    hmModuleRegistry* registry = (hmModuleRegistry*)user_data;
    hmClass* class_ref = HM_NULL;
    hmError err = hmModuleRegistryGetClassRefByModuleAndClassID(registry, metadata->module_id, metadata->class_id, &class_ref);
    if (err != HM_OK) {
        return HM_ERROR_INVALID_DATA;
    }
    HM_TRY(hmModuleValidateMethodDoesNotExist(class_ref, metadata->method_id, &metadata->name));
    hmMethod method;
    HM_TRY(hmCreateMethod(
        registry->allocator,
        metadata->method_id,
        &metadata->name,
        &metadata->signature,
        metadata->body.opcodes,
        metadata->body.size,
        &method
    ));
    hmString name;
    err = hmStringDuplicate(registry->allocator, &metadata->name, &name);
    if (err != HM_OK) {
        return hmMergeErrors(err, hmMethodDispose(&method));
    }
    /* hm_class, name are moved to hmModuleStoreMethod(..) */
    return hmModuleStoreMethod(class_ref, metadata->method_id, &name, &method);
}

static hmError hmSignatureResolve(hmSignature* signature, hmModuleRegistry* module_registry)
{
    return HM_OK;
}

static hmError hmMethodResolve(hmMethod* method, hmModuleRegistry* module_registry)
{
    return hmSignatureResolve(&method->signature, module_registry);
}

static hmError hmClassResolve_enumMethodsFunc(void* key, void* value, void* user_data)
{
    return hmMethodResolve((hmMethod*)value, (hmModuleRegistry*)user_data);
}

static hmError hmClassResolve(hmClass* hm_class, hmModuleRegistry* module_registry)
{
    return hmHashMapEnumerate(&hm_class->name_to_method_map, &hmClassResolve_enumMethodsFunc, module_registry);
}

static hmError hmModuleResolve_enumClassesFunc(void* key, void* value, void* user_data)
{
    return hmClassResolve((hmClass*)value, (hmModuleRegistry*)user_data);
}

static hmError hmModuleResolve(hmModule* module, hmModuleRegistry* module_registry)
{
    return hmHashMapEnumerate(&module->name_to_class_map, &hmModuleResolve_enumClassesFunc, module_registry);
}
