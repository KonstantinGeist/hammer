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
#include "type.h"

struct _hmAllocator;

/* A method of execution consists of: its signature (parameter types, return type), its method body (bytecode)
   and other auxiliary data. */
typedef struct {
    const char*          name;         /* The name of the method. The name should be unique in a given class. */
    hmArray/*<TypeRef>*/ params;       /* Typeref list for the parameters. */
    hmArray/*<Opcode>*/  opcodes;      /* Bytecode of the method to be interpreted. */
    hmTypeRef            return_value; /* The typeref of the returned value. Can be HM_TYPEKIND_VOID as well. */
} hmMethod;

/* A class is a collection of methods and properties bundled together. */
typedef struct _hmClass {
    const char* name;              /* The name of the class (NOT fully qualified, for example: "StringBuilder"). The name
                                      should be unique in a given module. */
    hmArray/*<hmMethod>*/ methods; /* List of methods this class has. The size of the array is determined by method_count. */
} hmClass;

/* A module is a collection of classes. */
typedef struct {
    const char*         name;    /* The name of the module. Should be unique in a given module registry. */
    hmArray/*<Class>*/  classes; /* The list of the classes. */
} hmModule;

/* A module registry is a collection of modules. It allows to register new modules. */
typedef struct {
    struct _hmAllocator* allocator;  /* The allocator which is used to allocate all metadata (classes, methods) in this registry.
                                        Generally, we want a super-fast bump pointer allocator because runtime metadata
                                        is generally static but consists of many small objects. */
    hmArray/*<Module>*/  modules;    /* The list of modules. */
} hmModuleRegistry;

/* Creates a new module registry. Every runtime owns a module registry where all modules are registered. */
hmError hmCreateModuleRegistry(struct _hmAllocator* allocator, hmModuleRegistry *in_registry);

/* Delete the module registry and all its data, including modules and internal bookkeeping data. */
hmError hmModuleRegistryDispose(hmModuleRegistry* registry);

/* Loads a module from a Hammer image denoted by the path on disk. After registering, all classes in the module
   are immediately usable. */
hmError hmModuleRegistryRegisterFromImage(hmModuleRegistry* registry, const char* image_path);

#endif /* HM_MODULE_H */
