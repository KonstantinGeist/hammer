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

#include "../common.h"
#include <core/utils.h>
#include <io/reader.h>

#include <string.h> /* for memcmp(..) */

#define SMALL_READ_BUFFER_SIZE 5
#define LARGE_READ_BUFFER_SIZE 1024
#define MEMORY_BUFFER_STRING "Hello, World"

static void create_memory_reader_and_allocator(hmReader* reader, hmAllocator* allocator)
{
    HM_TEST_INIT_ALLOC(allocator);
    HM_TEST_TRACK_OOM(allocator, HM_FALSE);
    hmError err = hmCreateMemoryReader(allocator, MEMORY_BUFFER_STRING, strlen(MEMORY_BUFFER_STRING), reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(allocator, HM_TRUE);
}

static void dispose_memory_reader_and_allocator(hmReader* reader, hmAllocator* allocator)
{
    hmError err = hmReaderClose(reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(allocator);
}

static void test_memory_reader_can_create_read_close()
{
    char read_buffer[SMALL_READ_BUFFER_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderRead(&reader, read_buffer, SMALL_READ_BUFFER_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(bytes_read == SMALL_READ_BUFFER_SIZE);
    HM_TEST_ASSERT(memcmp(read_buffer, "Hello", SMALL_READ_BUFFER_SIZE) == 0);
HM_TEST_ON_FINALIZE
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_can_create_seek_read_close()
{
    char read_buffer[SMALL_READ_BUFFER_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderSeek(&reader, 3);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmReaderRead(&reader, read_buffer, SMALL_READ_BUFFER_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(bytes_read == SMALL_READ_BUFFER_SIZE);
    HM_TEST_ASSERT(memcmp(read_buffer, "lo, W", SMALL_READ_BUFFER_SIZE) == 0);
HM_TEST_ON_FINALIZE
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_cant_seek_past_buffer()
{
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderSeek(&reader, 15);
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_ARGUMENT);
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_truncates_buffer_if_read_past_buffer()
{
    char read_buffer[SMALL_READ_BUFFER_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderSeek(&reader, 8);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmReaderRead(&reader, read_buffer, SMALL_READ_BUFFER_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(bytes_read == SMALL_READ_BUFFER_SIZE-1);
    HM_TEST_ASSERT(memcmp(read_buffer, "orld", SMALL_READ_BUFFER_SIZE - 1) == 0);
HM_TEST_ON_FINALIZE
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_ignores_zero_size_requests()
{
    char read_buffer[SMALL_READ_BUFFER_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderRead(&reader, read_buffer, 0, &bytes_read);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(bytes_read == 0);
    HM_TEST_ASSERT(read_buffer[0] == '\0');
HM_TEST_ON_FINALIZE
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_does_not_allow_to_read_past_buffer_impl(hm_nint buffer_size)
{
    char* read_buffer = hmAllocOnStack(buffer_size);
    hmZeroMemory(read_buffer, buffer_size);
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmReaderRead(&reader, read_buffer, buffer_size, &bytes_read);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    hm_nint buffer_string_length = strlen(MEMORY_BUFFER_STRING);
    hm_nint expected_bytes_read = buffer_size < buffer_string_length ? buffer_size : buffer_string_length;
    HM_TEST_ASSERT(bytes_read == expected_bytes_read);
    HM_TEST_ASSERT(memcmp(read_buffer, MEMORY_BUFFER_STRING, expected_bytes_read) == 0);
    err = hmReaderRead(&reader, read_buffer, buffer_size, &bytes_read);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    if (buffer_size < buffer_string_length) {
        HM_TEST_ASSERT(bytes_read != 0);
    } else {
        HM_TEST_ASSERT(bytes_read == 0);
    }
HM_TEST_ON_FINALIZE
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_does_not_allow_to_read_past_buffer()
{
    for (hm_nint i = 1; i <= LARGE_READ_BUFFER_SIZE; i++) {
        test_memory_reader_does_not_allow_to_read_past_buffer_impl(i);
    }
}

static void test_can_create_memory_reader_from_empty_string()
{
    hmAllocator allocator;
    hmReader reader;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateMemoryReader(&allocator, "", 0, &reader);
    HM_TEST_ASSERT_OK(err);
    char read_buffer[SMALL_READ_BUFFER_SIZE];
    hm_nint bytes_read;
    err = hmReaderRead(&reader, read_buffer, sizeof(read_buffer), &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == 0);
    err = hmReaderClose(&reader);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

HM_TEST_SUITE_BEGIN(readers)
    HM_TEST_RUN(test_memory_reader_can_create_read_close)
    HM_TEST_RUN(test_memory_can_create_seek_read_close)
    HM_TEST_RUN(test_memory_reader_cant_seek_past_buffer)
    HM_TEST_RUN(test_memory_reader_truncates_buffer_if_read_past_buffer)
    HM_TEST_RUN(test_memory_reader_ignores_zero_size_requests)
    HM_TEST_RUN(test_memory_reader_does_not_allow_to_read_past_buffer)
    HM_TEST_RUN_WITHOUT_OOM(test_can_create_memory_reader_from_empty_string)
HM_TEST_SUITE_END()
