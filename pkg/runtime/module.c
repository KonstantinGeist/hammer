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

#include <runtime/module.h>
#include <core/allocator.h>
#include <core/primitives.h>
#include <vendor/sqlite3/sqlite3.h>

static hmError hmModuleRegistryLoadModules(hmModuleRegistry* registry, sqlite3* db);
static hmError hmCreateModule(hmAllocator* allocator, hm_int32 module_id, hmString* name, hmModule* in_module);
static hmError hmModuleRegistryLoadClasses(hmModuleRegistry* registry, sqlite3* db);
static hmError hmModuleDispose(hmModule* module);
static hmError hmClassDispose(hmClass* hm_class);
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
    HM_OWNED(in_registry->name_to_module_map);
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

hmError hmModuleRegistryLoadFromImage(hmModuleRegistry* registry, hmString* image_path)
{
    if (!image_path) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmError err = HM_OK;
    sqlite3* db;
    int sqlite_err = sqlite3_open_v2(hmStringContent(image_path), &db, SQLITE_OPEN_READONLY, HM_NULL);
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

hmError hmModuleGetClassRefByName(hmModule* module, hmString* name, hmClass** out_class)
{
    void* class_ref;
    HM_TRY(hmHashMapGetRef(&module->name_to_class_map, name, &class_ref));
    *out_class = (hmClass*)class_ref;
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
    HM_MOVED(name, registry->name_to_module_map);
    HM_MOVED(module, registry->name_to_module_map);
    name_and_module_owned = HM_FALSE;
    void* module_ref;
    HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&registry->name_to_module_map, name, &module_ref));
    err = hmHashMapPut(&registry->module_id_to_module_ref_map, &module_id, &module_ref);
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

static hmError hmModuleRegistryValidateModuleDoesNotExist(hmModuleRegistry* registry, hm_int32 module_id, hmString* name_view)
{
    hm_bool found = hmHashMapContains(&registry->name_to_module_map, name_view);
    if (found) {
        return HM_ERROR_INVALID_IMAGE;
    }
    found = hmHashMapContains(&registry->module_id_to_module_ref_map, &module_id);
    if (found) {
        return HM_ERROR_INVALID_IMAGE;
    }
    return HM_OK;
}

static hmError hmModuleRegistryRegisterModule(hmModuleRegistry* registry, hm_int32 module_id, hmString* name_view)
{
    HM_TRY(hmModuleRegistryValidateModuleDoesNotExist(registry, module_id, name_view));
    hmModule module;
    HM_TRY(hmCreateModule(registry->allocator, module_id, name_view, &module));
    HM_OWNED(module);
    hmString name_key;
    hmError err = hmStringDuplicate(registry->allocator, name_view, &name_key);
    if (err != HM_OK) {
        return hmCombineErrors(err, hmModuleDispose(&module));
    }
    HM_MOVED(module, hmModuleRegistryStoreModule);
    HM_MOVED(name_key, hmModuleRegistryStoreModule);
    HM_TRY(hmModuleRegistryStoreModule(registry, module_id, &name_key, &module));
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
                    const char* name = (const char*)sqlite3_column_text(stmt, 1);
                    hmString name_view;
                    HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString(name, &name_view));
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

static hmError hmCreateModule(hmAllocator* allocator, hm_int32 module_id, hmString* name_view, hmModule* in_module)
{
    HM_TRY(hmStringDuplicate(allocator, name_view, &in_module->name));
    HM_OWNED(in_module->name);
    hmError err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmClassDisposeFunc, // value_dispose_func
        sizeof(hmClass),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        &in_module->name_to_class_map
    );
    if (err != HM_OK) {
        return hmCombineErrors(err, hmStringDispose(&in_module->name));
    }
    HM_MOVED(in_module->name, in_module);
    err = hmCreateHashMap(
        allocator,
        &hmInt32HashFunc,
        &hmInt32EqualsFunc,
        HM_NULL,            // key_dispose_func
        HM_NULL,            // value_dispose_func,
        sizeof(hm_int32),
        sizeof(hmClass*),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        &in_module->class_id_to_class_ref_map
    );
    if (err != HM_OK) {
        err = hmCombineErrors(err, hmStringDispose(&in_module->name));
        return hmCombineErrors(err, hmHashMapDispose(&in_module->name_to_class_map));
    }
    in_module->module_id = module_id;
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

static hmError hmCreateClass(hmAllocator* allocator, hm_int32 class_id, hmString* name_view, hmClass* in_class)
{
    HM_TRY(hmStringDuplicate(allocator, name_view, &in_class->name));
    HM_MOVED(in_class->name, in_class);
    in_class->class_id = class_id;
    return HM_OK;
}

static hmError hmModuleStoreClass(hmModule* module, hm_int32 class_id, hmString* name, hmClass* hm_class)
{
    HM_OWNED(name);
    HM_OWNED(hm_class);
    hmError err = HM_OK;
    hm_bool name_and_class_owned = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmHashMapPut(&module->name_to_class_map, name, hm_class));
    HM_MOVED(name, module->name_to_class_map);
    HM_MOVED(hm_class, module->name_to_class_map);
    name_and_class_owned = HM_FALSE;
    void* class_ref;
    HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&module->name_to_class_map, name, &class_ref));
    err = hmHashMapPut(&module->class_id_to_class_ref_map, &class_id, (hmClass*)class_ref);
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (name_and_class_owned) {
            err = hmCombineErrors(err, hmStringDispose(name));
            err = hmCombineErrors(err, hmClassDispose(hm_class));
        } else {
            err = hmCombineErrors(err, hmHashMapRemove(&module->name_to_class_map, name, HM_NULL));
            err = hmCombineErrors(err, hmHashMapRemove(&module->class_id_to_class_ref_map, &class_id, HM_NULL));
        }
    }
    return err;
}

static hmError hmModuleRegistryRegisterClass(hmModuleRegistry* registry, hm_int32 class_id, hm_int32 module_id, hmString* name_view)
{
    hmModule* module_ref;
    hmError err = hmModuleRegistryGetModuleRefByID(registry, module_id, &module_ref);
    if (err != HM_OK) {
        return HM_ERROR_INVALID_IMAGE;
    }
    HM_TRY(hmModuleValidateClassDoesNotExist(module_ref, class_id, name_view));
    hmClass hm_class;
    HM_TRY(hmCreateClass(registry->allocator, class_id, name_view, &hm_class));
    HM_OWNED(hm_class);
    hmString name_key;
    err = hmStringDuplicate(registry->allocator, name_view, &name_key);
    if (err != HM_OK) {
        return hmCombineErrors(err, hmClassDispose(&hm_class));
    }
    HM_MOVED(hm_class, hmModuleStoreClass);
    HM_MOVED(name_key, hmModuleStoreClass);
    HM_TRY(hmModuleStoreClass(module_ref, class_id, &name_key, &hm_class));
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
                    const char* name = (const char*)sqlite3_column_text(stmt, 2);
                    hmString name_view;
                    HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString(name, &name_view));
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
    err = hmCombineErrors(err, hmHashMapDispose(&module->name_to_class_map));
    return hmCombineErrors(err, hmHashMapDispose(&module->class_id_to_class_ref_map));
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
