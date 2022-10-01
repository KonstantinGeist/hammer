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

hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry)
{
    hmError err = hmCreateArray(allocator, sizeof(hmModule), HM_DEFAULT_ARRAY_CAPACITY, HM_NULL, &in_registry->modules);
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

// TODO
#include <stdio.h>

static int module_load_callback(void* user_data, int row_count, char** columns, char** names)
{
    hmModuleRegistry* registry = (hmModuleRegistry*)user_data;
    for (hm_nint i = 0; i < row_count; i++) {
        printf("%s: %s\n", columns[i], names[i]);
    }
    return 0;
}

hmError hmModuleRegistryRegisterFromImage(hmModuleRegistry* registry, const char* image_path)
{
    hmError err = HM_OK;
    sqlite3* db;
    int sqlite_err = sqlite3_open_v2(image_path, &db, SQLITE_OPEN_READONLY, HM_NULL);
    if (sqlite_err != SQLITE_OK) {
        return HM_ERROR_NOT_FOUND;
    }
    sqlite_err = sqlite3_exec(db, "SELECT name FROM module", &module_load_callback, registry, HM_NULL);
    if (sqlite_err != SQLITE_OK) {
        err = HM_ERROR_INVALID_IMAGE;
        goto exit;
    }
exit:
    sqlite_err = sqlite3_close(db);
    if (sqlite_err != SQLITE_OK) {
        return HM_ERROR_PLATFORM_DEPENDENT;
    }
    return err;
}
