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
#include "../src/string.h"

static void test_can_create_string()
{
    #define STRING_CONTENT "Hello, World!"
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmString string;
    err = hmCreateStringFromCString(&allocator, STRING_CONTENT, &string);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringLength(&string) == strlen(STRING_CONTENT));
    HM_TEST_ASSERT(strcmp(hmStringContent(&string), STRING_CONTENT) == 0);
    err = hmStringDispose(&string);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

void test_strings()
{
    test_can_create_string();
}
