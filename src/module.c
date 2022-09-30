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

#include "module.h"
#include "allocator.h"

hmError hmCreateModuleRegistry(hmAllocator* metadata_allocator, hmModuleRegistry* in_registry)
{
    in_registry->modules = HM_NULL;
    in_registry->metadata_allocator = metadata_allocator;
    in_registry->module_count = 0;
    return HM_OK;
}

hmError hmModuleRegistryDispose(hmModuleRegistry* registry)
{
    hmFree(registry->metadata_allocator, registry->modules);
    return HM_OK;
}
