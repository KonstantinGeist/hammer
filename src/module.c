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
#include "sqlite3.h"

static hmError hmModuleRegistryLoadModules(hmModuleRegistry* registry, sqlite3* db);
static hmError hmCreateModule(hmAllocator* allocator, hmString* name, hmModule* in_module);
static hmError hmModuleDispose(hmModule* module);
static hmError hmModuleDisposeFunc(void* object);
static hmError hmClassDisposeFunc(void* object);

hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry)
{
    hmError err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmModuleDisposeFunc, // value_dispose_func
        sizeof(hmModule),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        &in_registry->modules
    );
    if (err != HM_OK) {
        return err;
    }
    in_registry->allocator = allocator;
    return HM_OK;
}

hmError hmModuleRegistryDispose(hmModuleRegistry* registry)
{
    return hmHashMapDispose(&registry->modules);
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
    sqlite_err = sqlite3_close(db);
    if (sqlite_err != SQLITE_OK) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
    }
    return err;
}

hmError hmModuleRegistryGetModuleRefByName(hmModuleRegistry* registry, hmString* name, hmModule** out_module)
{
    void* module_ref;
    hmError err = hmHashMapGetRef(&registry->modules, name, &module_ref);
    if (err != HM_OK) {
        return err;
    }
    *out_module = (hmModule*)module_ref;
    return HM_OK;
}

// TODO
static hmError hmModuleRegistryRegisterModule(hmModuleRegistry* registry, hmString* name)
{
    hmModule module;
    hmError err = hmCreateModule(registry->allocator, name, &module);
    if (err != HM_OK) {
        return err;
    }
    hmString name_key;
    err = hmStringDuplicate(registry->allocator, name, &name_key);
    if (err != HM_OK) {
        // TODO dispose of module
        return err;
    }
    err = hmHashMapPut(&registry->modules, &name_key, &module);
    if (err != HM_OK) {
        // TODO dispose of module and name_key
        return err;
    }
    return HM_OK;
}

static hmError hmModuleRegistryLoadModules(hmModuleRegistry* registry, sqlite3* db)
{
    hmError err = HM_OK;
    sqlite3_stmt* stmt;
    int sqlite_err = sqlite3_prepare_v2(
        db,
        "SELECT name FROM module",
        -1,
        &stmt,
        HM_NULL
    );
    if (sqlite_err != SQLITE_OK) {
        err = HM_ERROR_INVALID_IMAGE;
        goto exit;
    }
    for (;;) {
        sqlite_err = sqlite3_step(stmt);
        switch (sqlite_err) {
            case SQLITE_ROW:
                {
                    const char* name = sqlite3_column_text(stmt, 0);
                    hmString name_view;
                    err = hmCreateStringViewFromCString(name, &name_view);
                    if (err != HM_OK) {
                        goto exit;
                    }
                    err = hmModuleRegistryRegisterModule(registry, &name_view);
                    if (err != HM_OK) {
                        goto exit;
                    }
                }
                break;
            case SQLITE_DONE:
                goto exit;
            case SQLITE_ERROR:
                err = HM_ERROR_INVALID_IMAGE;
                goto exit;
        }
    }
exit:
    sqlite_err = sqlite3_finalize(stmt);
    if (sqlite_err != SQLITE_OK) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
    }
    return err;
}

static hmError hmCreateModule(hmAllocator* allocator, hmString* name, hmModule* in_module)
{
    hmError err = hmStringDuplicate(allocator, name, &in_module->name);
    if (err != HM_OK) {
        return err;
    }
    err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmClassDisposeFunc, // value_dispose_func
        sizeof(hmClass),
        HM_DEFAULT_HASHMAP_CAPACITY,
        HM_DEFAULT_HASHMAP_LOAD_FACTOR,
        &in_module->classes
    );
    if (err != HM_OK) {
        hmError err2 = hmStringDispose(&in_module->name);
        if (err2 != HM_OK) {
            err = err2;
        }
        return err;
    }
    return HM_OK;
}

static hmError hmModuleDispose(hmModule* module)
{
    hmError err = hmStringDispose(&module->name);
    if (err != HM_OK) {
        return err;
    }
    return hmHashMapDispose(&module->classes);
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
