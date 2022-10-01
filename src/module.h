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

#include "common.h"
#include "array.h"
#include "string.h"
#include "type.h"

// TODO add a function to freeze a registry -- to allow for reading from multiple threads without locking
//      or all modules can be listed in the registry's constructor, making it immutable from the start
//      new modules can be added by chaining registries -- leaving original ones intact

struct _hmAllocator;

typedef struct {
    struct _hmAllocator* allocator;
    hmString             name;         /* The name of the method. The name should be unique in a given class. */
    hmArray/*<TypeRef>*/ params;
    hmArray/*<Opcode>*/  opcodes;      /* Bytecode of the method to be interpreted. */
    hmTypeRef            return_value; /* The typeref of the returned value. Can be HM_TYPEKIND_VOID as well. */
} hmMethod;

typedef struct _hmClass {
    struct _hmAllocator*  allocator;
    hmString              name;       /* The name of the class (NOT fully qualified, for example: "StringBuilder"). The name
                                         should be unique in a given module. */
    hmArray/*<hmMethod>*/ methods;
} hmClass;

typedef struct {
    hmString             name;      /* The name of the module. Should be unique in a given module registry. */
    hmArray/*<Class>*/   classes;
} hmModule;

typedef struct {
    struct _hmAllocator* allocator;
    hmArray/*<Module>*/  modules;
} hmModuleRegistry;

hmError hmCreateModuleRegistry(struct _hmAllocator* allocator, hmModuleRegistry *in_registry);
hmError hmModuleRegistryDispose(hmModuleRegistry* registry);

/* Loads a module from a Hammer image denoted by the path on disk. After registering, all classes in the module
   are immediately usable. */
hmError hmModuleRegistryLoadFromImage(hmModuleRegistry* registry, const char* image_path);

/* Returns a pointer to a module by its name in out_module. Returns HM_NULL if no module was found.
   Note that the owner of the module is the registry, do not attempt to dispose of the module or modify it. */
hmError hmModuleRegistryGetModuleByName(hmModuleRegistry* registry, const char* name, hmModule** out_module);

#define hmModuleName(module) (module)->name

#endif /* HM_MODULE_H */
