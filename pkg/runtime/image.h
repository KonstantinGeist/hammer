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

#ifndef HM_IMAGE_H
#define HM_IMAGE_H

#include <core/common.h>
#include <core/string.h>
#include <runtime/common.h>

typedef struct {
    hmString name;
    hm_metadata_id module_id;
} hmModuleMetadata;

typedef struct {
    hmString name;
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

typedef hmError (*hmEnumModuleMetadataInImageFunc)(hmModuleMetadata* metadata, void* user_data);
typedef hmError (*hmEnumClassMetadataInImageFunc)(hmClassMetadata* metadata, void* user_data);
typedef hmError (*hmEnumMethodMetadataInImageFunc)(hmMethodMetadata* metadata, void* user_data);

typedef struct hmImageLoader_ {
    hmError (*enumMetadata)(struct hmImageLoader_*          loader,
                            hmEnumModuleMetadataInImageFunc enum_modules_func_opt,
                            hmEnumClassMetadataInImageFunc  enum_classes_func_opt,
                            hmEnumMethodMetadataInImageFunc enum_methods_func_opt,
                            void*                           user_data);
    hmError (*dispose)(struct hmImageLoader_* loader);
    void* data; /* Loader-specific data. */
} hmImageLoader;

/* Creates an image loader which can load an image from a file specified by `image_path`. */
hmError hmCreateFileImageLoader(hmAllocator* allocator, hmString* image_path, hmImageLoader* in_image_loader);
hmError hmImageLoaderDispose(hmImageLoader* image_loader);
/* Enumerates metadata and calls provided callbacks in the order of the arguments.
   Can be used for constructing new modules, for inspecting metadata, etc.
   All callbacks can be HM_NULL if enumerating a specific object type (module, class or method) is not required.
   `user_data` can be used to pass additional context to the callbacks. */
hmError hmImageLoaderEnumMetadata(
    hmImageLoader*                  image_loader,
    hmEnumModuleMetadataInImageFunc enum_modules_func_opt,
    hmEnumClassMetadataInImageFunc  enum_classes_func_opt,
    hmEnumMethodMetadataInImageFunc enum_methods_func_opt,
    void* user_data
);

#endif /* HM_IMAGE_H */
