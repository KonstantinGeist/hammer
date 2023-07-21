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
#include <io/linereader.h>

#include <string.h> /* for strlen(..) */

#define LINE_READER_BUFFER_LARGE_SIZE 1024

static void create_line_reader_and_allocator(
    hmLineReader* line_reader,
    hmAllocator* allocator,
    const char* mem,
    char* buffer,
    hm_nint buffer_size
)
{
    HM_TEST_INIT_ALLOC(allocator);
    HM_TEST_TRACK_OOM(allocator, HM_FALSE);
    hmReader memory_reader;
    hmError err = hmCreateMemoryReader(allocator, mem, strlen(mem), &memory_reader);
    HM_TEST_ASSERT_OK(err);
    err = hmCreateLineReader(
        allocator,
        memory_reader,
        HM_TRUE, /* close_source_reader = HM_TRUE */
        buffer,
        buffer_size,
        line_reader
    );
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(allocator, HM_TRUE);
}

static void dispose_line_reader_and_allocator(hmLineReader* line_reader, hmAllocator* allocator)
{
    hmError err = hmLineReaderDispose(line_reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(allocator);
}

static void test_line_reader_supports_never_being_read()
{
    hmAllocator allocator;
    hmLineReader line_reader;
    char buffer[LINE_READER_BUFFER_LARGE_SIZE];
    create_line_reader_and_allocator(&line_reader, &allocator, "Hello, World!", buffer, sizeof(buffer));
    dispose_line_reader_and_allocator(&line_reader, &allocator);
}

static void test_line_reader_can_read_several_lines_impl(hm_nint buffer_size)
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    char* buffer = hmAllocOnStack(buffer_size);
    char* content = "Hello, World!\nGood bye, World!\nTrailing";
    hmReader memory_reader;
    hm_bool is_memory_reader_initialized = HM_FALSE,
            is_lines_initialized = HM_FALSE;
    hmError err = hmCreateMemoryReader(&allocator, content, strlen(content), &memory_reader);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_memory_reader_initialized = HM_TRUE;
    hmArray lines;
    err = hmReadAllLines(&allocator, memory_reader, buffer, buffer_size, &lines);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_lines_initialized = HM_TRUE;
    hmString* raw = hmArrayGetRaw(&lines, hmString);
    HM_TEST_ASSERT(hmStringEqualsToCString(&raw[0], "Hello, World!"));
    HM_TEST_ASSERT(hmStringEqualsToCString(&raw[1], "Good bye, World!"));
    HM_TEST_ASSERT(hmStringEqualsToCString(&raw[2], "Trailing"));
HM_TEST_ON_FINALIZE
    if (is_memory_reader_initialized) {
        err = hmReaderClose(&memory_reader);
        HM_TEST_ASSERT_OK(err);
    }
    if (is_lines_initialized) {
        err = hmArrayDispose(&lines);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

/* TODO add tests for various edge cases like empty reader, several \n\n right after another, a trailing (\n in the start/in the end) */

static void test_line_reader_can_read_several_lines()
{
    for (hm_nint i = 1; i < LINE_READER_BUFFER_LARGE_SIZE; i++) {
        test_line_reader_can_read_several_lines_impl(i);
    }
}

HM_TEST_SUITE_BEGIN(line_readers)
    HM_TEST_RUN(test_line_reader_supports_never_being_read)
    HM_TEST_RUN(test_line_reader_can_read_several_lines)
HM_TEST_SUITE_END()
