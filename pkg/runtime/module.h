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

#ifndef HM_MODULE_H
#define HM_MODULE_H

#include <core/common.h>
#include <core/string.h>
#include <collections/array.h>
#include <collections/hashmap.h>
#include <runtime/common.h>

struct _hmAllocator;

/* Modules store classes, and classes store methods, by using hashmaps. It's not superperformant per se (each lookup
   involves searching in a hashmap), however, high-level (HL) bytecode will be translated to low-level (LL) bytecode where
   pointers are directly stored in the LL bytecode, so it's not an issue at runtime (no dispatch will happen). Storing objects inside
   hashmaps has an advantage of it being more flexible: there can be "holes" in ID sequences which can happen if bytecode is patched.
   A disadvantage is that loading an image is slower (all the hashmaps must be populated) -- we can revisit it later if
   it proves to be too slow. */

typedef struct {
    hmString       name; /* The name of the method which should be unique in a given class. */
    hm_metadata_id method_id;
} hmMethod;

typedef struct {
    hmString       name;    /* The name of the class (NOT fully qualified, for example: "StringBuilder"). The name
                               should be unique in a given module. */
    hmHashMap      name_to_method_map;          /* hmHashMap<hmString, hmMethod>, for reflection */
    hmHashMap      method_id_to_method_ref_map; /* hmHashMap<hm_metadata_id, hmMethod*>, for linking */
    hm_metadata_id class_id;
} hmClass;

typedef struct {
    hmString       name;                      /* The name of the module. Should be unique in a given module registry. */
    hmHashMap      name_to_class_map;         /* hmHashMap<hmString, hmClass>, for reflection */
    hmHashMap      class_id_to_class_ref_map; /* hmHashMap<hm_metadata_id, hmClass*>, for linking */
    hm_metadata_id module_id;
} hmModule;

typedef struct {
    struct _hmAllocator* allocator;
    hmHashMap            name_to_module_map;          /* hmHashMap<hmString, hmModule>, for reflection */
    hmHashMap            module_id_to_module_ref_map; /* hmHashMap<hm_metadata_id, hmModule*>, for linking */
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

/* Similar to hmModuleRegistryGetModuleRefByName, but works with methods instead. */
hmError hmClassGetMethodRefByName(hmClass* hm_class, hmString* name, hmMethod** out_method);

#define hmModuleGetName(module) (module)->name
#define hmModuleGetID(module) (module)->module_id

#define hmClassGetName(hm_class) (hm_class)->name
#define hmClassGetID(hm_class) (hm_class)->class_id

#define hmMethodGetName(method) (method)->name
#define hmMethodGetID(method) (method)->method_id

#endif /* HM_MODULE_H */
