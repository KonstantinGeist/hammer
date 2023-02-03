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

#ifndef HM_METADATA_H
#define HM_METADATA_H

#include <core/common.h>
#include <core/string.h>
#include <runtime/common.h>

typedef struct {
    hmString       name;
    hm_metadata_id module_id;
} hmModuleMetadata;

typedef struct {
    hmString       name;
    hm_metadata_id class_id;
    hm_metadata_id module_id;
} hmClassMetadata;

typedef struct {
    const hm_uint8* opcodes; /* We don't use hmString to represent opcodes because they may contain HM_NULL in the middle
                                and hmString doesn't support that. */
    hm_method_size  size;
} hmMethodBodyMetadata;

typedef struct {
    hmString             name;
    hmString             signature; /* Signature is encoded similar to Java -- as a string. */
    hmMethodBodyMetadata body;
    hm_metadata_id       method_id;
    hm_metadata_id       class_id;
    hm_metadata_id       module_id;
} hmMethodMetadata;

typedef hmError (*hmEnumModuleMetadataFunc)(hmModuleMetadata* metadata, void* user_data);
typedef hmError (*hmEnumClassMetadataFunc)(hmClassMetadata* metadata, void* user_data);
typedef hmError (*hmEnumMethodMetadataFunc)(hmMethodMetadata* metadata, void* user_data);

typedef struct hmMetadataLoader_ {
    hmError (*enumMetadata)(struct hmMetadataLoader_* loader,
                            hmEnumModuleMetadataFunc  enum_modules_func_opt,
                            hmEnumClassMetadataFunc   enum_classes_func_opt,
                            hmEnumMethodMetadataFunc  enum_methods_func_opt,
                            void*                           user_data);
    hmError (*dispose)(struct hmMetadataLoader_* loader);
    void* data; /* Loader-specific data. */
} hmMetadataLoader;

/* Validates that the name is allowed as a name for a metadata object (module, class, method).
   We have very strict naming rules to make sure metadata names don't conflict with anything (signatures, emitted C code, etc.)
   Only 'a-Z', 'A-Z', digits, and '_' are allowed; additionally, a name can't start with a digit.
   Made public for tests (at least). */
hm_bool hmValidateMetadataName(hmString* name);
/* Creates a metadata loader which can load metadata from an image file specified by `image_path`. */
hmError hmCreateImageFileMetadataLoader(hmAllocator* allocator, hmString* image_path, hmMetadataLoader* in_metadata_loader);
hmError hmMetadataLoaderDispose(hmMetadataLoader* metadata_loader);
/* Enumerates metadata and calls provided callbacks in the order of the arguments.
   Can be used for constructing new modules, for inspecting metadata, etc.
   All callbacks can be HM_NULL if enumerating a specific object type (module, class or method) is not required.
   `user_data` can be used to pass additional context to the callbacks. */
hmError hmMetadataLoaderEnumMetadata(
    hmMetadataLoader*        metadata_loader,
    hmEnumModuleMetadataFunc enum_modules_func_opt,
    hmEnumClassMetadataFunc  enum_classes_func_opt,
    hmEnumMethodMetadataFunc enum_methods_func_opt,
    void* user_data
);

#endif /* HM_METADATA_H */
