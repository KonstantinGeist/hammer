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
    if (reader) {
        hmError err = hmReaderClose(reader);
        HM_TEST_ASSERT_OK(err);
    }
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
    HM_TEST_ASSERT(hmCompareMemory(read_buffer, "Hello", SMALL_READ_BUFFER_SIZE) == 0);
HM_TEST_ON_FINALIZE
    dispose_memory_reader_and_allocator(&reader, &allocator);
}

static void test_memory_reader_truncates_buffer_if_read_past_buffer()
{
    char read_buffer[SMALL_READ_BUFFER_SIZE] = {0};
    hm_nint bytes_read;
    hmAllocator allocator;
    hmReader reader;
    create_memory_reader_and_allocator(&reader, &allocator);
    hmError err = hmMemoryReaderSetPosition(&reader, 8);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    err = hmReaderRead(&reader, read_buffer, SMALL_READ_BUFFER_SIZE, &bytes_read);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(bytes_read == SMALL_READ_BUFFER_SIZE - 1);
    HM_TEST_ASSERT(hmMemoryReaderGetPosition(&reader) == 8 + SMALL_READ_BUFFER_SIZE - 1);
    HM_TEST_ASSERT(hmCompareMemory(read_buffer, "orld", SMALL_READ_BUFFER_SIZE - 1) == 0);
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
    HM_TEST_ASSERT(hmMemoryReaderGetPosition(&reader) == 0);
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
    HM_TEST_ASSERT(hmCompareMemory(read_buffer, MEMORY_BUFFER_STRING, expected_bytes_read) == 0);
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
    HM_TEST_ASSERT(hmMemoryReaderGetPosition(&reader) == 0);
    err = hmReaderClose(&reader);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_limited_reader_limits_reads()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmReader source_reader;
    hmError err = hmCreateMemoryReader(&allocator, "12345678", 8, &source_reader);
    HM_TEST_ASSERT_OK(err);
    hmReader limited_reader;
    err = hmCreateLimitedReader(&allocator, source_reader, HM_TRUE, 7, &limited_reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    char read_buffer[4] = {0};
    hm_nint bytes_read;
    err = hmReaderRead(&limited_reader, read_buffer, sizeof(read_buffer), &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == 4);
    HM_TEST_ASSERT(hmMemoryReaderGetPosition(&source_reader) == 4);
    HM_TEST_ASSERT(hmCompareMemory(read_buffer, "1234", 4) == 0);
    err = hmReaderRead(&limited_reader, read_buffer, sizeof(read_buffer), &bytes_read);
    HM_TEST_ASSERT(err == HM_ERROR_LIMIT_EXCEEDED); /* just hit the limit */
    HM_TEST_ASSERT(hmMemoryReaderGetPosition(&source_reader) == 7);
    HM_TEST_ASSERT(hmCompareMemory(read_buffer, "567", 3) == 0);
    err = hmReaderRead(&limited_reader, read_buffer, sizeof(read_buffer), &bytes_read);
    HM_TEST_ASSERT(err == HM_ERROR_LIMIT_EXCEEDED); /* repeated calls don't advance it */
    HM_TEST_ASSERT(hmMemoryReaderGetPosition(&source_reader) == 7);
    HM_TEST_ASSERT(hmCompareMemory(read_buffer, "567", 3) == 0);
    err = hmReaderClose(&limited_reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

typedef struct {
    hm_nint count;
} test_on_next_reader_context;

static hmError test_on_next_reader_func(hm_nint previous_reader_index, void* context_opt)
{
    test_on_next_reader_context* context = (test_on_next_reader_context*)context_opt;
    context->count++;
    return HM_OK;
}

static void test_composite_reader_reads_from_all_source_readers()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmReader source_reader1, source_reader2;
    hmError err = hmCreateMemoryReader(&allocator, "1234", 4, &source_reader1);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateMemoryReader(&allocator, "5678", 4, &source_reader2);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hm_bool is_composite_reader_initialized = HM_FALSE;
    hmReader source_readers[2] = {source_reader1, source_reader2};
    hm_bool close_source_readers[2] = {HM_TRUE, HM_TRUE};
    hmReader composite_reader;
    test_on_next_reader_context on_next_reader_context;
    on_next_reader_context.count = 0;
    err = hmCreateCompositeReader(
        &allocator,
        source_readers,
        close_source_readers,
        2,
        &test_on_next_reader_func,
        &on_next_reader_context,
        &composite_reader
    );
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_composite_reader_initialized = HM_TRUE;
    char buffer[32] = {0};
    hm_nint bytes_read = 0, total_bytes_read = 0;
    do {
        err = hmReaderRead(&composite_reader, buffer + total_bytes_read, sizeof(buffer), &bytes_read);
        HM_TEST_ASSERT_OK_OR_OOM(err);
        total_bytes_read += bytes_read;
    } while (bytes_read > 0);
    HM_TEST_ASSERT(hmCompareMemory(buffer, "12345678", 8) == 0);
    HM_TEST_ASSERT(on_next_reader_context.count == 2);
HM_TEST_ON_FINALIZE
    if (is_composite_reader_initialized) {
        err = hmReaderClose(&composite_reader);
        HM_TEST_ASSERT_OK(err);
    } else {
        err = hmReaderClose(&source_reader1);
        HM_TEST_ASSERT_OK(err);
        err = hmReaderClose(&source_reader2);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

HM_TEST_SUITE_BEGIN(readers)
    HM_TEST_RUN(test_memory_reader_can_create_read_close)
    HM_TEST_RUN(test_memory_reader_truncates_buffer_if_read_past_buffer)
    HM_TEST_RUN(test_memory_reader_ignores_zero_size_requests)
    HM_TEST_RUN(test_memory_reader_does_not_allow_to_read_past_buffer)
    HM_TEST_RUN_WITHOUT_OOM(test_can_create_memory_reader_from_empty_string)
    HM_TEST_RUN(test_limited_reader_limits_reads)
    HM_TEST_RUN(test_composite_reader_reads_from_all_source_readers)
HM_TEST_SUITE_END()
