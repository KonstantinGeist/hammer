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
#include "../src/allocator.h"
#include "../src/module.h"

static void test_can_load_modules()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmModuleRegistry module_registry;
    err = hmCreateModuleRegistry(&allocator, &module_registry);
    HM_TEST_ASSERT_OK(err);
    err = hmModuleRegistryRegisterFromImage(&module_registry, "../tests/data/module.hm");
    HM_TEST_ASSERT_OK(err);
    err = hmModuleRegistryDispose(&module_registry);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

void test_modules()
{
    test_can_load_modules();
}
