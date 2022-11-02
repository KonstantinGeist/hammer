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

#include "module.h"
#include "allocator.h"
#include "primitives.h"
#include "sqlite3.h"

static hmError hmModuleRegistryLoadModules(hmModuleRegistry* registry, sqlite3* db);
static hmError hmCreateModule(hmAllocator* allocator, hm_int32 module_id, hmString* name, hmModule* in_module);
static hmError hmModuleRegistryLoadClasses(hmModuleRegistry* registry, sqlite3* db);
static hmError hmModuleDispose(hmModule* module);
static hmError hmModuleDisposeFunc(void* object);
static hmError hmClassDisposeFunc(void* object);

hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry)
{
    HM_TRY(hmCreateHashMapWithStringKeys(
        allocator,
        &hmModuleDisposeFunc, // value_dispose_func
        sizeof(hmModule),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        &in_registry->name_to_module_map
    ));
    HM_OWNED(in_registry->name_to_module_map)
    hmError err = hmCreateHashMap(
        allocator,
        &hmInt32HashFunc,
        &hmInt32EqualsFunc,
        HM_NULL,            // key_dispose_func
        HM_NULL,            // value_dispose_func,
        sizeof(hm_int32),
        sizeof(hmModule*),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        &in_registry->module_id_to_module_ref_map
    );
    if (err != HM_OK) {
        return hmCombineErrors(err, hmHashMapDispose(&in_registry->name_to_module_map));
    }
    in_registry->allocator = allocator;
    HM_UNOWNED(in_registry->name_to_module_map)
    return HM_OK;
}

hmError hmModuleRegistryDispose(hmModuleRegistry* registry)
{
    hmError err = hmHashMapDispose(&registry->name_to_module_map);
    return hmCombineErrors(err, hmHashMapDispose(&registry->module_id_to_module_ref_map));
}

hmError hmModuleRegistryLoadFromImage(hmModuleRegistry* registry, const char* image_path)
{
    if (!image_path) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmError err = HM_OK;
    sqlite3* db;
    int sqlite_err = sqlite3_open_v2(image_path, &db, SQLITE_OPEN_READONLY, HM_NULL);
    if (sqlite_err != SQLITE_OK) {
        return HM_ERROR_NOT_FOUND;
    }
    err = hmModuleRegistryLoadModules(registry, db);
    err = hmCombineErrors(err, hmModuleRegistryLoadClasses(registry, db));
    sqlite_err = sqlite3_close(db);
    if (sqlite_err != SQLITE_OK) {
        err = hmCombineErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
    }
    return err;
}

hmError hmModuleRegistryGetModuleRefByName(hmModuleRegistry* registry, hmString* name, hmModule** out_module)
{
    void* module_ref;
    HM_TRY(hmHashMapGetRef(&registry->name_to_module_map, name, &module_ref));
    *out_module = (hmModule*)module_ref;
    return HM_OK;
}

static hmError hmModuleRegistryGetModuleRefByID(hmModuleRegistry* registry, hm_int32 module_id, hmModule** out_module)
{
    void* module_ref;
    HM_TRY(hmHashMapGet(&registry->module_id_to_module_ref_map, &module_id, &module_ref));
    *out_module = (hmModule*)module_ref;
    return HM_OK;
}

static hmError hmModuleRegistryStoreModule(hmModuleRegistry* registry, hm_int32 module_id, hmString* name, hmModule* module)
{
    HM_OWNED(name);
    HM_OWNED(module);
    hmError err = HM_OK;
    hm_bool name_and_module_owned = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmHashMapPut(&registry->name_to_module_map, name, module));
    HM_MOVED(name, registry->name_to_module_map)
    HM_MOVED(module, registry->name_to_module_map)
    name_and_module_owned = HM_FALSE;
    void* module_ref;
    HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&registry->name_to_module_map, name, &module_ref));
    err = hmHashMapPut(&registry->module_id_to_module_ref_map, &module_id, (hmModule*)&module_ref);
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (name_and_module_owned) {
            err = hmCombineErrors(err, hmStringDispose(name));
            err = hmCombineErrors(err, hmModuleDispose(module));
        } else {
            err = hmCombineErrors(err, hmHashMapRemove(&registry->name_to_module_map, name, HM_NULL));
            err = hmCombineErrors(err, hmHashMapRemove(&registry->module_id_to_module_ref_map, &module_id, HM_NULL));
        }
    }
    return err;
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

static hmError hmModuleRegistryRegisterModule(hmModuleRegistry* registry, hm_int32 module_id, hmString* name)
{
    HM_TRY(hmModuleRegistryValidateModuleDoesNotExist(registry, module_id, name));
    hmModule module;
    HM_TRY(hmCreateModule(registry->allocator, module_id, name, &module));
    HM_OWNED(module)
    hmString name_key;
    hmError err = hmStringDuplicate(registry->allocator, name, &name_key);
    if (err != HM_OK) {
        return hmCombineErrors(err, hmModuleDispose(&module));
    }
    HM_OWNED(name_key)
    HM_TRY(hmModuleRegistryStoreModule(registry, module_id, &name_key, &module));
    HM_MOVED(module, hmModuleRegistryStoreModule)
    HM_MOVED(name_key, hmModuleRegistryStoreModule)
    return HM_OK;
}

static hmError hmModuleRegistryLoadModules(hmModuleRegistry* registry, sqlite3* db)
{
    hmError err = HM_OK;
    sqlite3_stmt* stmt;
    int sqlite_err = sqlite3_prepare_v2(
        db,
        "SELECT module_id, name FROM module",
        -1,
        &stmt,
        HM_NULL
    );
    if (sqlite_err != SQLITE_OK) {
        err = HM_ERROR_INVALID_IMAGE;
        HM_FINALIZE;
    }
    for (;;) {
        sqlite_err = sqlite3_step(stmt);
        switch (sqlite_err) {
            case SQLITE_ROW:
                {
                    hm_int32 module_id = sqlite3_column_int(stmt, 0);
                    const char* name = sqlite3_column_text(stmt, 1);
                    hmString name_view;
                    HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString(name, &name_view));
                    HM_TEMP_VIEW(name_view)
                    HM_TRY_OR_FINALIZE(err, hmModuleRegistryRegisterModule(registry, module_id, &name_view));
                }
                break;
            case SQLITE_DONE:
                HM_FINALIZE;
            case SQLITE_ERROR:
                err = HM_ERROR_INVALID_IMAGE;
                HM_FINALIZE;
        }
    }
HM_ON_FINALIZE
    sqlite_err = sqlite3_finalize(stmt);
    if (sqlite_err != SQLITE_OK) {
        err = hmCombineErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
    }
    return err;
}

static hmError hmCreateModule(hmAllocator* allocator, hm_int32 module_id, hmString* name, hmModule* in_module)
{
    HM_TRY(hmStringDuplicate(allocator, name, &in_module->name));
    HM_OWNED(in_module->name)
    hmError err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmClassDisposeFunc, // value_dispose_func
        sizeof(hmClass),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        &in_module->classes
    );
    if (err != HM_OK) {
        return hmCombineErrors(err, hmStringDispose(&in_module->name));
    }
    HM_MOVED(in_module->name, in_module)
    in_module->module_id = module_id;
    return HM_OK;
}

// TODO check if class under same class_id, name already exists inside a given module; also check if module_id actually exists
static hmError hmModuleRegistryRegisterClass(hmModuleRegistry* registry, hm_int32 class_id, hm_int32 module_id, hmString* name)
{
    /*hmClass klass;
    HM_TRY(hmCreateClass(registry->allocator, class_id, module_id, name, &klass));
    HM_OWNED(klass)
    hmString name_key;
    hmError err = hmStringDuplicate(registry->allocator, name, &name_key);
    if (err != HM_OK) {
        return hmCombineErrors(err, hmModuleDispose(&klass));
    }
    HM_OWNED(name_key)
    err = hmHashMapPut(&registry->name_to_module_map, &name_key, &module);
    if (err != HM_OK) {
        err = hmCombineErrors(err, hmStringDispose(&name_key));
        err = hmCombineErrors(err, hmModuleDispose(&klass));
        return err;
    }
    HM_MOVED(module, registry->name_to_module_map)
    HM_MOVED(name_key, registry->name_to_module_map)*/
    return HM_OK;
}

static hmError hmModuleRegistryLoadClasses(hmModuleRegistry* registry, sqlite3* db)
{
    hmError err = HM_OK;
    sqlite3_stmt* stmt;
    int sqlite_err = sqlite3_prepare_v2(
        db,
        "SELECT class_id, module_id, name FROM class",
        -1,
        &stmt,
        HM_NULL
    );
    if (sqlite_err != SQLITE_OK) {
        err = HM_ERROR_INVALID_IMAGE;
        HM_FINALIZE;
    }
    for (;;) {
        sqlite_err = sqlite3_step(stmt);
        switch (sqlite_err) {
            case SQLITE_ROW:
                {
                    hm_int32 class_id = sqlite3_column_int(stmt, 0);
                    hm_int32 module_id = sqlite3_column_int(stmt, 1);
                    const char* name = sqlite3_column_text(stmt, 2);
                    hmString name_view;
                    HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString(name, &name_view));
                    HM_TEMP_VIEW(name_view)
                    HM_TRY_OR_FINALIZE(err, hmModuleRegistryRegisterClass(registry, class_id, module_id, &name_view));
                }
                break;
            case SQLITE_DONE:
                HM_FINALIZE;
            case SQLITE_ERROR:
                err = HM_ERROR_INVALID_IMAGE;
                HM_FINALIZE;
        }
    }
HM_ON_FINALIZE
    sqlite_err = sqlite3_finalize(stmt);
    if (sqlite_err != SQLITE_OK) {
        err = hmCombineErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
    }
    return err;
}

static hmError hmModuleDispose(hmModule* module)
{
    hmError err = hmStringDispose(&module->name);
    return hmCombineErrors(err, hmHashMapDispose(&module->classes));
}

static hmError hmModuleDisposeFunc(void* object)
{
    hmModule* module = (hmModule*)object;
    return hmModuleDispose(module);
}

static hmError hmClassDisposeFunc(void* object)
{
    // TODO
    return HM_OK;
}
