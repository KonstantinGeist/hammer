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

#define READ_BUF_SIZE 5

static void create_memory_reader(hmAllocator* allocator, hmReader* reader)
{
    const char* str = "Hello, World";
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateMemoryReader(str, 13, allocator, reader);
    HM_TEST_ASSERT_OK(err);
}

static void dispose_memory_reader_and_allocator(hmAllocator* allocator, hmReader* reader)
{
    hmError err = hmReaderClose(reader);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_memory_reader_can_create_read_close()
{
    char read_buf[READ_BUF_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader(&allocator, &reader);
    hmError err = hmReaderRead(&reader, &read_buf[0], READ_BUF_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == READ_BUF_SIZE);
    HM_TEST_ASSERT(memcmp(read_buf, "Hello", READ_BUF_SIZE) == 0);
    dispose_memory_reader_and_allocator(&allocator, &reader);
}

static void test_memory_can_create_seek_read_close()
{
    char read_buf[READ_BUF_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader(&allocator, &reader);
    hmError err = hmReaderSeek(&reader, 3);
    HM_TEST_ASSERT_OK(err);
    err = hmReaderRead(&reader, &read_buf[0], READ_BUF_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == READ_BUF_SIZE);
    HM_TEST_ASSERT(memcmp(read_buf, "lo, w", READ_BUF_SIZE) == 0);
    dispose_memory_reader_and_allocator(&allocator, &reader);
}

void test_readers()
{
    test_memory_reader_can_create_read_close();
    test_memory_can_create_seek_read_close();
}
