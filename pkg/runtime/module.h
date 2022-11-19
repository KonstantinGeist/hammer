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

#ifndef HM_MODULE_H
#define HM_MODULE_H

#include <core/common.h>
#include <collections/array.h>
#include <collections/hashmap.h>
#include <core/string.h>
#include "type.h"

struct _hmAllocator;

typedef struct _hmClass {
    hm_int32              class_id;
    hmString              name;      /* The name of the class (NOT fully qualified, for example: "StringBuilder"). The name
                                        should be unique in a given module. */
} hmClass;

typedef struct {
    hm_int32             module_id;
    hmString             name;      /* The name of the module. Should be unique in a given module registry. */
    hmHashMap            name_to_class_map;         /* hmHashMap<hmString, hmClass> */
    hmHashMap            class_id_to_class_ref_map; /* hmHashMap<hm_int32, hmClass*> */
} hmModule;

typedef struct {
    struct _hmAllocator* allocator;
    hmHashMap            name_to_module_map;          /* hmHashMap<hmString, hmModule> */
    hmHashMap            module_id_to_module_ref_map; /* hmHashMap<hm_int32, hmModule*> */
} hmModuleRegistry;

hmError hmCreateModuleRegistry(struct _hmAllocator* allocator, hmModuleRegistry *in_registry);
hmError hmModuleRegistryDispose(hmModuleRegistry* registry);

/* Loads a module from a Hammer image denoted by the path on disk. After registering, all classes in the module
   are immediately usable. */
hmError hmModuleRegistryLoadFromImage(hmModuleRegistry* registry, hmString* image_path);

/* Returns a pointer to a module by its name in out_module. Returns HM_NULL if no module was found.
   Note that the owner of the module is the registry, do not attempt to dispose of the module or modify it. */
hmError hmModuleRegistryGetModuleRefByName(hmModuleRegistry* registry, hmString* name, hmModule** out_module);

/* Similar to hmModuleRegistryGetModuleRefByName, but works with classes instead. */
hmError hmModuleGetClassRefByName(hmModule* module, hmString* name, hmClass** out_class);

#define hmModuleGetName(module) (module)->name
#define hmModuleGetID(module) (module)->module_id

#define hmClassGetName(hm_class) (hm_class)->name
#define hmClassGetID(hm_class) (hm_class)->class_id

#endif /* HM_MODULE_H */
