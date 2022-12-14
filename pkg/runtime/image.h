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

/* Enumerates metadata in a given Hammer image on disk and calls provided callbacks in the order of the arguments.
   Can be used for constructing new modules, for inspecting metadata, etc.
   All callbacks can be HM_NULL if enumerating a specific object type (module, class or method) is not required.
   `user_data` can be used to pass additional context to the callbacks. */
hmError hmEnumMetadataInImage(
    hmString* image_path,
    hmEnumModuleMetadataInImageFunc enum_modules_func,
    hmEnumClassMetadataInImageFunc enum_classes_func,
    hmEnumMethodMetadataInImageFunc enum_methods_func,
    void* user_data
);

#endif /* HM_IMAGE_H */
