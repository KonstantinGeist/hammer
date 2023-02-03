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

#ifndef HM_MODULE_REGISTRY_H
#define HM_MODULE_REGISTRY_H

#include <core/allocator.h>
#include <collections/hashmap.h>
#include <runtime/metadata.h>

typedef struct {
    hmAllocator* allocator;
    hmHashMap    modules; /* hmHashMap<hm_metadata_id, hmModule*> */
} hmModuleRegistry;

/* A module registry is where all modules and their classes are registered and stored. Typically, there should
   be only one module registry per runtime instance. */
hmError hmCreateModuleRegistry(hmAllocator* allocator, hmModuleRegistry* in_registry);
hmError hmModuleRegistryDispose(hmModuleRegistry* registry);
/* Loads a module using the provided metadata loader. After registering, all classes in the module are immediately usable.
   Note that this method is not thread-safe, so any active workers must be temporarily suspended before calling it. */
hmError hmModuleRegistryLoad(hmModuleRegistry* registry, hmMetadataLoader* metadata_loader);

#endif /* HM_MODULE_REGISTRY_H */
