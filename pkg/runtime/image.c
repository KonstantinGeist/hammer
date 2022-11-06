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

#include <runtime/image.h>
#include <vendor/sqlite3/sqlite3.h>

static hmError hmEnumerateModulesInImage(sqlite3* db, hmEnumerateModulesFunc enumerate_modules_func, void* user_data);
static hmError hmEnumerateClassesInImage(sqlite3* db, hmEnumerateClassesFunc enumerate_classes_func, void* user_data);

hmError hmEnumerateMetadataInImage(
    hmString* image_path,
    hmEnumerateModulesFunc enumerate_modules_func,
    hmEnumerateClassesFunc enumerate_classes_func,
    void* user_data
)
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
    err = hmEnumerateModulesInImage(db, enumerate_modules_func, user_data);
    err = hmCombineErrors(err, hmEnumerateClassesInImage(db, enumerate_classes_func, user_data));
    sqlite_err = sqlite3_close(db);
    if (sqlite_err != SQLITE_OK) {
        err = hmCombineErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
    }
    return err;
}

static hmError hmEnumerateModulesInImage(sqlite3* db, hmEnumerateModulesFunc enumerate_modules_func, void* user_data)
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
                    HM_TRY_OR_FINALIZE(err, enumerate_modules_func(module_id, &name_view, user_data));
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

static hmError hmEnumerateClassesInImage(sqlite3* db, hmEnumerateClassesFunc enumerate_classes_func, void* user_data)
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
                    HM_TRY_OR_FINALIZE(err, enumerate_classes_func(class_id, module_id, &name_view, user_data));
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
