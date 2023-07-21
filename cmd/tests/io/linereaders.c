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

HM_TEST_SUITE_BEGIN(line_readers)
    HM_TEST_RUN(test_line_reader_supports_never_being_read)
HM_TEST_SUITE_END()
