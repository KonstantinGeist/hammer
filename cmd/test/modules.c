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

#include "common.h"
#include <runtime/module.h>
#include <core/string.h>

static void create_module_registry_and_allocator(hmModuleRegistry* module_registry, hmAllocator* allocator)
{
    HM_TEST_INIT_ALLOC(allocator);
    HM_TEST_TRACK_OOM(allocator, HM_FALSE);
    hmError err = hmCreateModuleRegistry(allocator, module_registry);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(allocator, HM_TRUE);
}

static void dispose_module_registry_and_allocator(hmModuleRegistry* module_registry, hmAllocator* allocator)
{
    hmError err = hmModuleRegistryDispose(module_registry);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(allocator);
}

static hmError load_image(hmModuleRegistry* module_registry)
{
    hmString image_path;
    HM_TRY(hmCreateStringViewFromCString("../cmd/test/data/modules.hma", &image_path));
    return hmModuleRegistryLoadFromImage(module_registry, &image_path);
}

static void test_can_load_existing_module()
{
    #define CORE_MODULE_NAME "core"
    #define POINT_CLASS_NAME "Point"
    #define FOO_METHOD_NAME "foo"
    hmAllocator allocator;
    hmModuleRegistry module_registry;
    create_module_registry_and_allocator(&module_registry, &allocator);
    hmError err = load_image(&module_registry);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmString core_module_name;
    err = hmCreateStringViewFromCString(CORE_MODULE_NAME, &core_module_name);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmModule* module = HM_NULL;
    err = hmModuleRegistryGetModuleRefByName(&module_registry, &core_module_name, &module);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(module != HM_NULL);
    HM_TEST_ASSERT(hmStringEqualsToCString(&hmModuleGetName(module), CORE_MODULE_NAME));
    HM_TEST_ASSERT(hmModuleGetID(module) == 1);
    hmString point_class_name;
    err = hmCreateStringViewFromCString(POINT_CLASS_NAME, &point_class_name);
    hmClass* hm_class = HM_NULL;
    err = hmModuleGetClassRefByName(module, &point_class_name, &hm_class);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(hm_class != HM_NULL);
    HM_TEST_ASSERT(hmStringEqualsToCString(&hmClassGetName(hm_class), POINT_CLASS_NAME));
    HM_TEST_ASSERT(hmClassGetID(hm_class) == 1);
    hmString foo_method_name;
    err = hmCreateStringViewFromCString(FOO_METHOD_NAME, &foo_method_name);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmMethod* method = HM_NULL;
    err = hmClassGetMethodRefByName(hm_class, &foo_method_name, &method);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(method != HM_NULL);
    HM_TEST_ASSERT(hmStringEqualsToCString(&hmMethodGetName(method), FOO_METHOD_NAME));
    HM_TEST_ASSERT(hmMethodGetID(method) == 1);
HM_TEST_ON_FINALIZE
    dispose_module_registry_and_allocator(&module_registry, &allocator);
}

static void test_cannot_load_non_existing_module()
{
    #define NON_EXISTING_MODULE_NAME "non_existing"
    hmAllocator allocator;
    hmModuleRegistry module_registry;
    create_module_registry_and_allocator(&module_registry, &allocator);
    hmError err = load_image(&module_registry);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmString non_existing_module_name;
    err = hmCreateStringViewFromCString(NON_EXISTING_MODULE_NAME, &non_existing_module_name);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hmModule* module = HM_NULL;
    err = hmModuleRegistryGetModuleRefByName(&module_registry, &non_existing_module_name, &module);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
    HM_TEST_ASSERT(module == HM_NULL);
HM_TEST_ON_FINALIZE
    dispose_module_registry_and_allocator(&module_registry, &allocator);
}

HM_TEST_SUITE_BEGIN(modules)
    HM_TEST_RUN(test_can_load_existing_module)
    HM_TEST_RUN(test_cannot_load_non_existing_module)
HM_TEST_SUITE_END()
