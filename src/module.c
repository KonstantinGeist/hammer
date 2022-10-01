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
static hmError hmCreateModule(hmAllocator* allocator, const char* name, hmModule* in_module);
static hmError hmModuleDispose(hmModule* module);
static hmError hmModuleDisposeFunc(void* object);

hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry)
{
    hmError err = hmCreateArray(
        allocator,
        sizeof(hmModule),
        HM_DEFAULT_ARRAY_CAPACITY,
        &hmModuleDisposeFunc,
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
    return hmArrayDispose(&registry->modules);
}

// TODO validation step after loading
hmError hmModuleRegistryLoadFromImage(hmModuleRegistry* registry, const char* image_path)
{
    hmError err = HM_OK;
    sqlite3* db;
    int sqlite_err = sqlite3_open_v2(image_path, &db, SQLITE_OPEN_READONLY, HM_NULL);
    if (sqlite_err != SQLITE_OK) {
        return HM_ERROR_NOT_FOUND;
    }
    err = hmModuleRegistryLoadModules(registry, db);
    if (err != HM_OK) {
        goto exit;
    }
exit:
    sqlite_err = sqlite3_close(db);
    if (sqlite_err != SQLITE_OK) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
    }
    return err;
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
                    hmModule module;
                    err = hmCreateModule(registry->allocator, name, &module);
                    if (err != HM_OK) {
                        goto exit;
                    }
                    err = hmArrayAdd(&registry->modules, &module);
                    if (err != HM_OK) {
                        hmError err2 = hmModuleDispose(&module);
                        if (err2 != HM_OK) {
                            err = err2;
                        }
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

static hmError hmCreateModule(hmAllocator* allocator, const char* name, hmModule* in_module)
{
    hmError err = hmCreateStringFromCString(allocator, name, &in_module->name);
    if (err != HM_OK) {
        return err;
    }
    err = hmCreateArray(allocator, sizeof(hmString), HM_DEFAULT_ARRAY_CAPACITY, HM_NULL, &in_module->classes);
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
    return hmArrayDispose(&module->classes);
}

static hmError hmModuleDisposeFunc(void* object)
{
    hmModule* module = (hmModule*)object;
    return hmModuleDispose(module);
}
