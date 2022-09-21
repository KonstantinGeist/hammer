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
#include "../src/reader.h"

// TODO
/*#define SETUP_MEMORY_READER \
    const char* str = "Hello, World"; \
    hmReader reader; \
    hmError err = hmCreateMemoryReader(str, 13, allocator, &reader); \
    HM_TEST_ASSERT(err == HM_OK);*/

// TODO
static void test_memory_reader_can_create_and_close()
{

}

void test_reader()
{
    test_memory_reader_can_create_and_close();
}
