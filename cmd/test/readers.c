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
#include <io/reader.h>

#define READ_BUF_SIZE 5
#define MEMORY_BUF_STRING "Hello, World"

static void create_memory_reader_and_allocator(hmReader* reader, hmAllocator* allocator)
{
    hmError err = hmCreateSystemAllocator(allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateMemoryReader(allocator, MEMORY_BUF_STRING, strlen(MEMORY_BUF_STRING), reader);
    HM_TEST_ASSERT_OK(err);
}

static void dispose_memory_reader_and_allocator(hmReader* reader, hmAllocator* allocator)
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
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderRead(&reader, &read_buf[0], READ_BUF_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == READ_BUF_SIZE);
    HM_TEST_ASSERT(memcmp(read_buf, "Hello", READ_BUF_SIZE) == 0);
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_can_create_seek_read_close()
{
    char read_buf[READ_BUF_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderSeek(&reader, 3);
    HM_TEST_ASSERT_OK(err);
    err = hmReaderRead(&reader, &read_buf[0], READ_BUF_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == READ_BUF_SIZE);
    HM_TEST_ASSERT(memcmp(read_buf, "lo, W", READ_BUF_SIZE) == 0);
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_cant_seek_past_buffer()
{
    char read_buf[READ_BUF_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderSeek(&reader, 15);
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_ARGUMENT);
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_truncates_buffer_if_read_past_buffer()
{
    char read_buf[READ_BUF_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderSeek(&reader, 8);
    HM_TEST_ASSERT_OK(err);
    err = hmReaderRead(&reader, &read_buf[0], READ_BUF_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == READ_BUF_SIZE-1);
    HM_TEST_ASSERT(memcmp(read_buf, "orld", READ_BUF_SIZE-1) == 0);
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_ignores_zero_size_requests()
{
    char read_buf[READ_BUF_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderRead(&reader, &read_buf[0], 0, &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == 0);
    HM_TEST_ASSERT(read_buf[0] == '\0');
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

void test_readers()
{
    test_memory_reader_can_create_read_close();
    test_memory_can_create_seek_read_close();
    test_memory_reader_cant_seek_past_buffer();
    test_memory_reader_truncates_buffer_if_read_past_buffer();
    test_memory_reader_ignores_zero_size_requests();
}
