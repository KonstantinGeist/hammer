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

hmError hmImageLoaderDispose(hmImageLoader* image_loader)
{
    return image_loader->dispose(image_loader);
}

hmError hmImageLoaderEnumMetadata(
    hmImageLoader*                  image_loader,
    hmEnumModuleMetadataInImageFunc enum_modules_func_opt,
    hmEnumClassMetadataInImageFunc  enum_classes_func_opt,
    hmEnumMethodMetadataInImageFunc enum_methods_func_opt,
    void* user_data
)
{
    return image_loader->enumMetadata(
        image_loader,
        enum_modules_func_opt,
        enum_classes_func_opt,
        enum_methods_func_opt,
        user_data
    );
}

/* ************************ */
/*      FileImageLoader.    */
/* ************************ */

typedef struct {
    hmAllocator* allocator;
    hmString     image_path;
} hmFileImageLoaderData;

static hmError hmFileImageLoaderEnumModules(sqlite3* db, hmEnumModuleMetadataInImageFunc enum_modules_func, void* user_data);
static hmError hmFileImageLoaderEnumClasses(sqlite3* db, hmEnumClassMetadataInImageFunc enum_classes_func, void* user_data);
static hmError hmFileImageLoaderEnumMethods(sqlite3* db, hmEnumMethodMetadataInImageFunc enum_methods_func, void* user_data);
static hmError hmFileImageLoaderGetMetadataIdFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hm_metadata_id* out_id);
static hmError hmFileImageLoaderGetMethodSizeFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hm_method_size* out_size);
static hmError hmFileImageLoaderGetStringViewFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hmString* in_string_view);
static hmError hmFileImageLoaderGetBlobFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, const hm_uint8** out_blob);
static hmError hmFileImageLoader_enumMetadata(
    hmImageLoader*                  image_loader,
    hmEnumModuleMetadataInImageFunc enum_modules_func_opt,
    hmEnumClassMetadataInImageFunc  enum_classes_func_opt,
    hmEnumMethodMetadataInImageFunc enum_methods_func_opt,
    void* user_data
);
static hmError hmFileImageLoader_dispose(hmImageLoader* image_loader);

hmError hmCreateFileImageLoader(hmAllocator* allocator, hmString* image_path, hmImageLoader* in_image_loader)
{
    if (!image_path) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmFileImageLoaderData* data = (hmFileImageLoaderData*)hmAlloc(allocator, sizeof(hmFileImageLoaderData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmStringDuplicate(allocator, image_path, &data->image_path));
    data->allocator = allocator;
    in_image_loader->enumMetadata = &hmFileImageLoader_enumMetadata;
    in_image_loader->dispose = hmFileImageLoader_dispose;
    in_image_loader->data = data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        hmFree(allocator, data);
    }
    return err;
}

static hmError hmFileImageLoader_dispose(hmImageLoader* image_loader)
{
    hmFileImageLoaderData* data = (hmFileImageLoaderData*)image_loader->data;
    hmError err = hmStringDispose(&data->image_path);
    hmFree(data->allocator, data);
    return err;
}

static hmError hmFileImageLoader_enumMetadata(
    hmImageLoader*                  image_loader,
    hmEnumModuleMetadataInImageFunc enum_modules_func_opt,
    hmEnumClassMetadataInImageFunc  enum_classes_func_opt,
    hmEnumMethodMetadataInImageFunc enum_methods_func_opt,
    void* user_data
)
{
    hmFileImageLoaderData* data = (hmFileImageLoaderData*)image_loader->data;
    hmError err = HM_OK;
    sqlite3* db;
    int sqlite_err = sqlite3_open_v2(hmStringGetCString(&data->image_path), &db, SQLITE_OPEN_READONLY, HM_NULL);
    if (sqlite_err != SQLITE_OK) {
        return HM_ERROR_NOT_FOUND;
    }
    if (enum_modules_func_opt) {
        err = hmFileImageLoaderEnumModules(db, enum_modules_func_opt, user_data);
    }
    if (enum_classes_func_opt) {
        err = hmMergeErrors(err, hmFileImageLoaderEnumClasses(db, enum_classes_func_opt, user_data));
    }
    if (enum_methods_func_opt) {
        err = hmMergeErrors(err, hmFileImageLoaderEnumMethods(db, enum_methods_func_opt, user_data));
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

static hmError hmFileImageLoaderEnumModules(sqlite3* db, hmEnumModuleMetadataInImageFunc enum_modules_func_opt, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT module_id, name FROM module")
        hmModuleMetadata metadata;
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetMetadataIdFromStatement(db, stmt, 0, &metadata.module_id));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetStringViewFromStatement(db, stmt, 1, &metadata.name));
        HM_TRY_OR_FINALIZE(err, enum_modules_func_opt(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}

static hmError hmFileImageLoaderEnumClasses(sqlite3* db, hmEnumClassMetadataInImageFunc enum_classes_func_opt, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT class_id, module_id, name FROM class")
        hmClassMetadata metadata;
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetMetadataIdFromStatement(db, stmt, 0, &metadata.class_id));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetMetadataIdFromStatement(db, stmt, 1, &metadata.module_id));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetStringViewFromStatement(db, stmt, 2, &metadata.name));
        HM_TRY_OR_FINALIZE(err, enum_classes_func_opt(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}

static hmError hmFileImageLoaderEnumMethods(sqlite3* db, hmEnumMethodMetadataInImageFunc enum_methods_func_opt, void* user_data)
{
    HM_BEGIN_SQLITE3_QUERY("SELECT method_id, class_id, module_id, name, signature, code, length(code) AS code_length FROM method")
        hmMethodMetadata metadata;
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetMetadataIdFromStatement(db, stmt, 0, &metadata.method_id));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetMetadataIdFromStatement(db, stmt, 1, &metadata.class_id));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetMetadataIdFromStatement(db, stmt, 2, &metadata.module_id));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetStringViewFromStatement(db, stmt, 3, &metadata.name));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetStringViewFromStatement(db, stmt, 4, &metadata.signature));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetBlobFromStatement(db, stmt, 5, &metadata.body.opcodes));
        HM_TRY_OR_FINALIZE(err, hmFileImageLoaderGetMethodSizeFromStatement(db, stmt, 6, &metadata.body.size));
        HM_TRY_OR_FINALIZE(err, enum_methods_func_opt(&metadata, user_data));
    HM_END_SQLITE3_QUERY()
}

static hm_bool hmHasSqlite3ErrorOccurred(sqlite3* db) {
    int error_code = sqlite3_errcode(db);
    return error_code != SQLITE_OK && error_code != SQLITE_ROW && error_code != SQLITE_DONE;
}

static hmError hmFileImageLoaderGetMetadataIdFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hm_metadata_id* out_id)
{
    sqlite3_int64 id = sqlite3_column_int64(stmt, column_index);
    if (!id && hmHasSqlite3ErrorOccurred(db)) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    if (id < HM_MIN_METADATA_ID || id > HM_MAX_METADATA_ID) {
        return HM_ERROR_INVALID_IMAGE;
    }
    *out_id = (hm_metadata_id)id;
    return HM_OK;
}

static hmError hmFileImageLoaderGetMethodSizeFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hm_method_size* out_size)
{
    sqlite3_int64 size = sqlite3_column_int64(stmt, column_index);
    if (!size && hmHasSqlite3ErrorOccurred(db)) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    if (size < HM_MIN_METHOD_SIZE || size > HM_MAX_METHOD_SIZE) {
        return HM_ERROR_INVALID_IMAGE;
    }
    *out_size = (hm_method_size)size;
    return HM_OK;
}

static hmError hmFileImageLoaderGetStringViewFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, hmString* in_string_view)
{
    const char* name = (const char*)sqlite3_column_text(stmt, column_index);
    if (!name) {
        return hmHasSqlite3ErrorOccurred(db) ? HM_ERROR_OUT_OF_MEMORY : HM_ERROR_INVALID_IMAGE;
    }
    return hmCreateStringViewFromCString(name, in_string_view);
}

static hmError hmFileImageLoaderGetBlobFromStatement(sqlite3* db, sqlite3_stmt* stmt, int column_index, const hm_uint8** out_blob)
{
    const hm_uint8* blob = (const hm_uint8*)sqlite3_column_blob(stmt, column_index);
    if (!blob) {
        return hmHasSqlite3ErrorOccurred(db) ? HM_ERROR_OUT_OF_MEMORY : HM_ERROR_INVALID_IMAGE;
    }
    *out_blob = blob;
    return HM_OK;
}
