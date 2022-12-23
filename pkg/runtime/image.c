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

#include <runtime/image.h>
#include <vendor/sqlite3/sqlite3.h>

static hmError hmEnumModulesInImage(sqlite3* db, hmEnumModuleMetadataInImageFunc enum_modules_func, void* user_data);
static hmError hmEnumClassesInImage(sqlite3* db, hmEnumClassMetadataInImageFunc enum_classes_func, void* user_data);
static hmError hmEnumMethodsInImage(sqlite3* db, hmEnumMethodMetadataInImageFunc enum_methods_func, void* user_data);

hmError hmEnumMetadataInImage(
    hmString* image_path,
    hmEnumModuleMetadataInImageFunc enum_modules_func,
    hmEnumClassMetadataInImageFunc enum_classes_func,
    hmEnumMethodMetadataInImageFunc enum_methods_func,
    void* user_data
)
{
    if (!image_path) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmError err = HM_OK;
    sqlite3* db;
    int sqlite_err = sqlite3_open_v2(hmStringGetRaw(image_path), &db, SQLITE_OPEN_READONLY, HM_NULL);
    if (sqlite_err != SQLITE_OK) {
        return HM_ERROR_NOT_FOUND;
    }
    if (enum_modules_func) {
        err = hmEnumModulesInImage(db, enum_modules_func, user_data);
    }
    if (enum_classes_func) {
        err = hmMergeErrors(err, hmEnumClassesInImage(db, enum_classes_func, user_data));
    }
    if (enum_methods_func) {
        err = hmMergeErrors(err, hmEnumMethodsInImage(db, enum_methods_func, user_data));
    }
    sqlite_err = sqlite3_close(db);
    if (sqlite_err != SQLITE_OK) {
        err = hmMergeErrors(err, HM_ERROR_PLATFORM_DEPENDENT);
    }
    return err;
}

#define HM_BEGIN_SQLITE3_QUERY(query) \
    hmError err = HM_OK; \
    sqlite3_stmt* stmt; \
    int sqlite_err = sqlite3_prepare_v2( \
        db, \
        query, \
        -1, \
        &stmt, \
        HM_NULL \
    ); \
    if (sqlite_err != SQLITE_OK) { \
        err = HM_ERROR_INVALID_IMAGE; \
        HM_FINALIZE; \
    } \
    for (;;) { \
        sqlite_err = sqlite3_step(stmt); \
        switch (sqlite_err) { \
            case SQLITE_ROW: \
                {

#define HM_END_SQLITE3_QUERY() \
                } \
                break; \
            case SQLITE_DONE: \
                HM_FINALIZE; \
            case SQLITE_ERROR: \
                err = HM_ERROR_INVALID_IMAGE; \
                HM_FINALIZE; \
        } \
    } \
HM_ON_FINALIZE \
    sqlite_err = sqlite3_finalize(stmt); \
    if (sqlite_err != SQLITE_OK) { \
        err = hmMergeErrors(err, HM_ERROR_PLATFORM_DEPENDENT); \
    } \
    return err;

static hmError hmEnumModulesInImage(sqlite3* db, hmEnumModuleMetadataInImageFunc enum_modules_func, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT module_id, name FROM module")
        hmModuleMetadata metadata;
        metadata.module_id = sqlite3_column_int(stmt, 0);
        const char* name = (const char*)sqlite3_column_text(stmt, 1);
        HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString(name, &metadata.name));
        HM_TRY_OR_FINALIZE(err, enum_modules_func(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}

static hmError hmEnumClassesInImage(sqlite3* db, hmEnumClassMetadataInImageFunc enum_classes_func, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT class_id, module_id, name FROM class")
        hmClassMetadata metadata;
        metadata.class_id = sqlite3_column_int(stmt, 0);
        metadata.module_id = sqlite3_column_int(stmt, 1);
        const char* name = (const char*)sqlite3_column_text(stmt, 2);
        HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString(name, &metadata.name));
        HM_TRY_OR_FINALIZE(err, enum_classes_func(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}

static hmError hmEnumMethodsInImage(sqlite3* db, hmEnumMethodMetadataInImageFunc enum_methods_func, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT method_id, class_id, name, signature, code, length(code) AS code_length FROM method")
        hmMethodMetadata metadata;
        metadata.method_id = sqlite3_column_int(stmt, 0);
        metadata.class_id = sqlite3_column_int(stmt, 1);
        const char* name = (const char*)sqlite3_column_text(stmt, 2);
        HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString(name, &metadata.name));
        const char* signature = (const char*)sqlite3_column_text(stmt, 3);
        HM_TRY_OR_FINALIZE(err, hmCreateStringViewFromCString(signature, &metadata.signature));
        metadata.body.opcodes = (char*)sqlite3_column_blob(stmt, 4);
        metadata.body.size = sqlite3_column_int(stmt, 5);
        HM_TRY_OR_FINALIZE(err, enum_methods_func(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}
