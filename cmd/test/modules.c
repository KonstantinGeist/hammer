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

#include "common.h"
#include <core/allocator.h>
#include <runtime/module.h>
#include <core/string.h>

static void create_module_registry_and_allocator(hmModuleRegistry* module_registry, hmAllocator* allocator)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateModuleRegistry(allocator, module_registry);
    HM_TEST_ASSERT_OK(err);
    hmString image_path;
    err = hmCreateStringViewFromCString("../cmd/test/data/modules.hma", &image_path);
    HM_TEST_ASSERT_OK(err);
    err = hmModuleRegistryLoadFromImage(module_registry, &image_path);
    HM_TEST_ASSERT_OK(err);
}

static void dispose_module_registry_and_allocator(hmModuleRegistry* module_registry, hmAllocator* allocator)
{
    hmError err = hmModuleRegistryDispose(module_registry);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_can_load_existing_module()
{
    #define CORE_MODULE_NAME "core"
    #define POINT_CLASS_NAME "Point"
    hmAllocator allocator;
    hmModuleRegistry module_registry;
    create_module_registry_and_allocator(&module_registry, &allocator);
    hmString core_module_name;
    hmError err = hmCreateStringViewFromCString(CORE_MODULE_NAME, &core_module_name);
    HM_TEST_ASSERT_OK(err);
    hmModule* module = HM_NULL;
    err = hmModuleRegistryGetModuleRefByName(&module_registry, &core_module_name, &module);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(module != HM_NULL);
    HM_TEST_ASSERT(hmStringEqualsToCString(&hmModuleName(module), CORE_MODULE_NAME));
    HM_TEST_ASSERT(hmModuleID(module) == 1);
    hmString point_class_name;
    err = hmCreateStringViewFromCString(POINT_CLASS_NAME, &point_class_name);
    hmClass* hm_class = HM_NULL;
    err = hmModuleGetClassRefByName(module, &point_class_name, &hm_class);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hm_class != HM_NULL);
    HM_TEST_ASSERT(hmStringEqualsToCString(&hmClassName(hm_class), POINT_CLASS_NAME));
    HM_TEST_ASSERT(hmClassID(hm_class) == 1);
    dispose_module_registry_and_allocator(&module_registry, &allocator);
}

static void test_cannot_load_non_existing_module()
{
    #define NON_EXISTING_MODULE_NAME "non_existing"
    hmAllocator allocator;
    hmModuleRegistry module_registry;
    create_module_registry_and_allocator(&module_registry, &allocator);
    hmString non_existing_module_name;
    hmError err = hmCreateStringViewFromCString(NON_EXISTING_MODULE_NAME, &non_existing_module_name);
    HM_TEST_ASSERT_OK(err);
    hmModule* module = HM_NULL;
    err = hmModuleRegistryGetModuleRefByName(&module_registry, &non_existing_module_name, &module);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
    HM_TEST_ASSERT(module == HM_NULL);
    dispose_module_registry_and_allocator(&module_registry, &allocator);
}

void test_modules()
{
    HM_TEST_LOG("Modules...");
    test_can_load_existing_module();
    test_cannot_load_non_existing_module();
}
