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

#ifndef HM_IMAGE_H
#define HM_IMAGE_H

#include <core/common.h>
#include <core/string.h>

typedef hmError (*hmEnumerateModulesFunc)(hm_int32 module_id, hmString* name, void* user_data);
typedef hmError (*hmEnumerateClassesFunc)(hm_int32 class_id, hm_int32 module_id, hmString* name, void* user_data);

/* Enumerates metadata in a given Hammer image on disk and calls provided callbacks in the order of the arguments.
   Can be used for constructing new modules, for inspecting metadata, etc.
   `user_data` can be used to pass additional context to the callbacks. */
hmError hmEnumerateMetadataInImage(
    hmString* image_path,
    hmEnumerateModulesFunc enumerate_modules_func,
    hmEnumerateClassesFunc enumerate_classes_func,
    void* user_data
);

#endif /* HM_IMAGE_H */
