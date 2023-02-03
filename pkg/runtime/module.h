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
#include <runtime/metadata.h>

/* Modules store classes, and classes store methods, by using hashmaps. It's not superperformant per se (each lookup
   involves searching in a hashmap), however, high-level (HL) bytecode will be translated to low-level (LL) bytecode where
   pointers will be directly stored in the LL bytecode, so it's not an issue at runtime (no dispatch will happen). Storing objects inside
   hashmaps has an advantage of it being more flexible: there can be "holes" in ID sequences which can happen if bytecode is patched.
   A disadvantage is that loading metadata is slower (all the hashmaps must be populated and searched) -- we can revisit it later if
   it proves to be too slow. We also assume our runtime is used in servers where startup is not as important as request processing speed.
   Note also that our pattern to embed allocator references in every struct inflates structure sizes considerably, which makes
   our code more flexible but is bad for memory locality; however we assume that at runtime, metadata will be rarely touched. */

typedef struct {
    struct hmClass_* return_class;
    hmArray          param_classes; /* hmArray<hmClass*> */
    hmString         desc;          /* Unresolved signature description in the text form as stored in the metadata. */
    hm_bool          is_resolved;   /* Initially, it's HM_FALSE. If HM_TRUE (after resolution), values in `param_classes`
                                       and `return_class` are meaningful. */
} hmSignature;

typedef struct {
    hm_uint8*      opcodes;
    hm_method_size size;
} hmMethodBody;

typedef struct {
    hmAllocator*   allocator;
    hmString       name;      /* The name of the method which should be unique in a given class. */
    hmSignature    signature; /* Describes the parameters and the return type. */
    hmMethodBody   hl_body;   /* High-level bytecode: it will be compiled to low-level bytecode during method resolution. */
    hm_metadata_id method_id;
} hmMethod;

typedef struct hmClass_ {
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
    hmAllocator* allocator;
    hmHashMap    name_to_module_map;          /* hmHashMap<hmString, hmModule>, for reflection */
    hmHashMap    module_id_to_module_ref_map; /* hmHashMap<hm_metadata_id, hmModule*>, for linking */
    hmArray      module_ids_to_resolve;       /* hmArray<hm_metadata_id>, a list of modules to resolve (populated during
                                                 metadata loading, accounted for during resolution). */
} hmModuleRegistry;

/* A module registry is where all modules and their classes are registered and stored. Typically, there should
   be only one module registry per runtime instance.
   Thread safety: as a registry can be shared by multiple concurrent theads of execution, it should not be modified
   when user code is running, unless otherwise noted. */
hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry);
hmError hmModuleRegistryDispose(hmModuleRegistry* registry);
/* Loads a module using the provided metadata loader. After registering, all classes in the module are immediately usable.
   Note that this method is not thread-safe, so workers must be suspended before calling it. */
hmError hmModuleRegistryLoad(hmModuleRegistry* registry, hmMetadataLoader* metadata_loader);
/* Returns a pointer to a module by its name in out_module. Returns HM_NULL if no module was found.
   Note that the owner of the module is the registry, do not attempt to dispose of the module or modify it. */
hmError hmModuleRegistryGetModuleRefByName(hmModuleRegistry* registry, hmString* name, hmModule** out_module);
/* Similar to hmModuleRegistryGetModuleRefByName, but works with classes instead. */
hmError hmModuleGetClassRefByName(hmModule* module, hmString* name, hmClass** out_class);
/* Similar to hmModuleRegistryGetModuleRefByName, but works with methods instead. */
hmError hmClassGetMethodRefByName(hmClass* hm_class, hmString* name, hmMethod** out_method);
/* Validates that the name is allowed as a name for a metadata object (module, class, method).
   We have very strict naming rules to make sure metadata names don't conflict with anything (signatures, emitted C code, etc.)
   Only 'a-Z', 'A-Z', digits, and '_' are allowed; additionally, a name can't start with a digit.
   Made public for tests (at least). */
hm_bool hmIsValidMetadataName(hmString* name);

#define hmModuleGetName(module) (module)->name
#define hmModuleGetID(module) (module)->module_id
#define hmClassGetName(hm_class) (hm_class)->name
#define hmClassGetID(hm_class) (hm_class)->class_id
#define hmMethodGetName(method) (method)->name
#define hmMethodGetID(method) (method)->method_id

#endif /* HM_MODULE_H */
