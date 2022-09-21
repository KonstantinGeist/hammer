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

/* A module is a collection of classes. */
typedef struct {
    const char* name;        /* The name of the module. Should be unique in a given module registry. */
    hmClass*    classes;     /* The list of the classes. The size is determined by class_count. */
    hm_nint     class_count; /* Specifies the number of classes this module has. */
} hmModule;

/* A module registry is a collection of modules. It allows to register new modules. */
typedef struct {
    hmModule*    modules;             /* The list of modules. The size is determined by module_count. */
    hmAllocator* metadata_allocator;  /* The allocator which is used to allocate all metadata in this registry.
                                         Generally, we want a super-fast bump pointer allocator because runtime metadata
                                         is generally static but consists of many small objects. */
    hm_nint      module_count;        /* Specifies the number of modules this registry has. */
} hmModuleRegistry;

#endif /* HM_MODULE_H */
