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

#include <runtime/metadata.h>
#include <vendor/sqlite3/sqlite3.h>

static hm_bool hmIsValidMetadataName(hmString* name);

hm_bool hmValidateMetadataName(hmString* name)
{
    hm_bool is_valid = hmIsValidMetadataName(name);
    if (!is_valid) {
        return HM_ERROR_INVALID_DATA;
    }
    return HM_OK;
}

hmError hmMetadataLoaderDispose(hmMetadataLoader* metadata_loader)
{
    return metadata_loader->dispose(metadata_loader);
}

hmError hmMetadataLoaderEnumMetadata(
    hmMetadataLoader*        metadata_loader,
    hmEnumModuleMetadataFunc enum_modules_func_opt,
    hmEnumClassMetadataFunc  enum_classes_func_opt,
    hmEnumMethodMetadataFunc enum_methods_func_opt,
    void* user_data
)
{
    return metadata_loader->enumMetadata(
        metadata_loader,
        enum_modules_func_opt,
        enum_classes_func_opt,
        enum_methods_func_opt,
        user_data
    );
}

static hm_bool hmIsValidMetadataName(hmString* name)
{
    hm_nint length = hmStringGetLengthInBytes(name);
    if (!length) {
        return HM_FALSE;
    }
    char* chars = hmStringGetChars(name);
    for (hm_nint i = 0; i < length; i++) {
        char c = chars[i];
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

/* ******************************** */
/*      ImageFileMetadataLoader.    */
/* ******************************** */

typedef struct {
    hmAllocator* allocator;
    hmString     image_path;
} hmImageFileMetadataLoaderData;

static hmError hmImageFileMetadataLoaderEnumModules(sqlite3* db, hmEnumModuleMetadataFunc enum_modules_func, void* user_data);
static hmError hmImageFileMetadataLoaderEnumClasses(sqlite3* db, hmEnumClassMetadataFunc enum_classes_func, void* user_data);
static hmError hmImageFileMetadataLoaderEnumMethods(sqlite3* db, hmEnumMethodMetadataFunc enum_methods_func, void* user_data);
static hmError hmSqlite3GetMetadataIdFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hm_metadata_id* out_id);
static hmError hmSqlite3GetMethodSizeFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hm_method_size* out_size);
static hmError hmSqlite3GetStringViewFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hmString* in_string_view);
static hmError hmSqlite3GetBlobFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, const hm_uint8** out_blob);
static hmError hmImageFileMetadataLoader_enumMetadata(
    hmMetadataLoader*        metadata_loader,
    hmEnumModuleMetadataFunc enum_modules_func_opt,
    hmEnumClassMetadataFunc  enum_classes_func_opt,
    hmEnumMethodMetadataFunc enum_methods_func_opt,
    void* user_data
);
static hmError hmImageFileMetadataLoader_dispose(hmMetadataLoader* metadata_loader);

hmError hmCreateImageFileMetadataLoader(hmAllocator* allocator, hmString* image_path, hmMetadataLoader* in_metadata_loader)
{
    if (!image_path) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmImageFileMetadataLoaderData* data = (hmImageFileMetadataLoaderData*)hmAlloc(allocator, sizeof(hmImageFileMetadataLoaderData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmStringDuplicate(allocator, image_path, &data->image_path));
    data->allocator = allocator;
    in_metadata_loader->enumMetadata = &hmImageFileMetadataLoader_enumMetadata;
    in_metadata_loader->dispose = &hmImageFileMetadataLoader_dispose;
    in_metadata_loader->data = data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        hmFree(allocator, data);
    }
    return err;
}

static hmError hmImageFileMetadataLoader_dispose(hmMetadataLoader* metadata_loader)
{
    hmImageFileMetadataLoaderData* data = (hmImageFileMetadataLoaderData*)metadata_loader->data;
    hmError err = hmStringDispose(&data->image_path);
    hmFree(data->allocator, data);
    return err;
}

static hmError hmImageFileMetadataLoader_enumMetadata(
    hmMetadataLoader*        metadata_loader,
    hmEnumModuleMetadataFunc enum_modules_func_opt,
    hmEnumClassMetadataFunc  enum_classes_func_opt,
    hmEnumMethodMetadataFunc enum_methods_func_opt,
    void* user_data
)
{
    hmImageFileMetadataLoaderData* data = (hmImageFileMetadataLoaderData*)metadata_loader->data;
    hmError err = HM_OK;
    sqlite3* db;
    int sqlite_err = sqlite3_open_v2(hmStringGetCString(&data->image_path), &db, SQLITE_OPEN_READONLY, HM_NULL);
    if (sqlite_err != SQLITE_OK) {
        return HM_ERROR_NOT_FOUND;
    }
    if (enum_modules_func_opt) {
        err = hmImageFileMetadataLoaderEnumModules(db, enum_modules_func_opt, user_data);
    }
    if (enum_classes_func_opt) {
        err = hmMergeErrors(err, hmImageFileMetadataLoaderEnumClasses(db, enum_classes_func_opt, user_data));
    }
    if (enum_methods_func_opt) {
        err = hmMergeErrors(err, hmImageFileMetadataLoaderEnumMethods(db, enum_methods_func_opt, user_data));
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
        err = HM_ERROR_INVALID_DATA; \
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
                err = HM_ERROR_INVALID_DATA; \
                HM_FINALIZE; \
        } \
    } \
HM_ON_FINALIZE \
    sqlite_err = sqlite3_finalize(stmt); \
    if (sqlite_err != SQLITE_OK) { \
        err = hmMergeErrors(err, HM_ERROR_PLATFORM_DEPENDENT); \
    } \
    return err;

static hmError hmImageFileMetadataLoaderEnumModules(sqlite3* db, hmEnumModuleMetadataFunc enum_modules_func_opt, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT module_id, name FROM module")
        hmModuleMetadata metadata;
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetMetadataIdFromStatement(db, stmt, 0, &metadata.module_id));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetStringViewFromStatement(db, stmt, 1, &metadata.name));
        HM_TRY_OR_FINALIZE(err, enum_modules_func_opt(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}

static hmError hmImageFileMetadataLoaderEnumClasses(sqlite3* db, hmEnumClassMetadataFunc enum_classes_func_opt, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT class_id, module_id, name FROM class")
        hmClassMetadata metadata;
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetMetadataIdFromStatement(db, stmt, 0, &metadata.class_id));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetMetadataIdFromStatement(db, stmt, 1, &metadata.module_id));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetStringViewFromStatement(db, stmt, 2, &metadata.name));
        HM_TRY_OR_FINALIZE(err, enum_classes_func_opt(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}

static hmError hmImageFileMetadataLoaderEnumMethods(sqlite3* db, hmEnumMethodMetadataFunc enum_methods_func_opt, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT method_id, class_id, module_id, name, signature, code, length(code) AS code_length FROM method")
        hmMethodMetadata metadata;
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetMetadataIdFromStatement(db, stmt, 0, &metadata.method_id));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetMetadataIdFromStatement(db, stmt, 1, &metadata.class_id));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetMetadataIdFromStatement(db, stmt, 2, &metadata.module_id));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetStringViewFromStatement(db, stmt, 3, &metadata.name));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetStringViewFromStatement(db, stmt, 4, &metadata.signature));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetBlobFromStatement(db, stmt, 5, &metadata.body.opcodes));
        HM_TRY_OR_FINALIZE(err, hmSqlite3GetMethodSizeFromStatement(db, stmt, 6, &metadata.body.size));
        HM_TRY_OR_FINALIZE(err, enum_methods_func_opt(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}

static hm_bool hmHasSqlite3ErrorOccurred(sqlite3* db) {
    int error_code = sqlite3_errcode(db);
    return error_code != SQLITE_OK && error_code != SQLITE_ROW && error_code != SQLITE_DONE;
}

static hmError hmSqlite3GetMetadataIdFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hm_metadata_id* out_id)
{
    sqlite3_int64 id = sqlite3_column_int64(stmt, column_index);
    if (!id && hmHasSqlite3ErrorOccurred(db)) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    if (id < HM_MIN_METADATA_ID || id > HM_MAX_METADATA_ID) {
        return HM_ERROR_INVALID_DATA;
    }
    *out_id = (hm_metadata_id)id;
    return HM_OK;
}

static hmError hmSqlite3GetMethodSizeFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hm_method_size* out_size)
{
    sqlite3_int64 size = sqlite3_column_int64(stmt, column_index);
    if (!size && hmHasSqlite3ErrorOccurred(db)) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    if (size < HM_MIN_METHOD_SIZE || size > HM_MAX_METHOD_SIZE) {
        return HM_ERROR_INVALID_DATA;
    }
    *out_size = (hm_method_size)size;
    return HM_OK;
}

static hmError hmSqlite3GetStringViewFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hmString* in_string_view)
{
    const char* name = (const char*)sqlite3_column_text(stmt, column_index);
    if (!name) {
        return hmHasSqlite3ErrorOccurred(db) ? HM_ERROR_OUT_OF_MEMORY : HM_ERROR_INVALID_DATA;
    }
    return hmCreateStringViewFromCString(name, in_string_view);
}

static hmError hmSqlite3GetBlobFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, const hm_uint8** out_blob)
{
    const hm_uint8* blob = (const hm_uint8*)sqlite3_column_blob(stmt, column_index);
    if (!blob) {
        return hmHasSqlite3ErrorOccurred(db) ? HM_ERROR_OUT_OF_MEMORY : HM_ERROR_INVALID_DATA;
    }
    *out_blob = blob;
    return HM_OK;
}
