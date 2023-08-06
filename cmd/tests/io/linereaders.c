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

#define LINE_READER_BUFFER_SIZE 128
#define LINE_READER_MAX_LINE_COUNT 16

static char* line_reader_lines[] = {
    "Hello, World!",
    "Goodbye, World!",
    "Trailing"
};

static void create_line_reader_and_allocator(
    hmLineReader* line_reader,
    hmAllocator*  allocator,
    const char*   mem,
    char*         buffer,
    hm_nint       buffer_size,
    hm_bool       has_crlf_newlines
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
        has_crlf_newlines,
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
    char buffer[LINE_READER_BUFFER_SIZE];
    create_line_reader_and_allocator(&line_reader, &allocator, "Hello, World!", buffer, sizeof(buffer), HM_FALSE);
    dispose_line_reader_and_allocator(&line_reader, &allocator);
}

static void create_line_reader_content(hmAllocator* allocator, hm_nint line_count, hm_bool is_crlf, hmString* in_line)
{
    hmStringBuilder string_builder;
    hmError err = hmCreateStringBuilder(allocator, &string_builder);
    HM_TEST_ASSERT_OK(err);
    for (hm_nint i = 0; i < line_count; i++) {
        err = hmStringBuilderAppendCString(&string_builder, line_reader_lines[i % (sizeof(line_reader_lines) / sizeof(char*))]);
        HM_TEST_ASSERT_OK(err);
        if (i < line_count - 1) {
            if (is_crlf) {
                err = hmStringBuilderAppendCString(&string_builder, "\r");
                HM_TEST_ASSERT_OK(err);                
            }
            err = hmStringBuilderAppendCString(&string_builder, "\n");
            HM_TEST_ASSERT_OK(err);
        }
    }
    err = hmStringBuilderToString(&string_builder, HM_NULL, in_line);
    HM_TEST_ASSERT_OK(err);
    err = hmStringBuilderDispose(&string_builder);
    HM_TEST_ASSERT_OK(err);
}

static void test_line_reader_can_read_several_lines_impl(hm_nint buffer_size, hm_nint line_count)
{
    for (hm_nint j = 0; j < 2; j++) {
        hm_bool has_crlf_newlines = j == 0;
        hmAllocator allocator;
        HM_TEST_INIT_ALLOC(&allocator);
        HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
        char* buffer = hmAllocOnStack(buffer_size);
        hmString content;
        create_line_reader_content(&allocator, line_count, has_crlf_newlines, &content);
        const char* c_content = hmStringGetCString(&content);
        hmReader memory_reader;
        hm_bool is_lines_initialized = HM_FALSE;
        hmError err = hmCreateMemoryReader(&allocator, c_content, strlen(c_content), &memory_reader);
        HM_TEST_ASSERT_OK(err);
        HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
        hmArray lines;
        err = hmReadAllLines(
            &allocator,
            memory_reader,
            buffer,
            buffer_size,
            has_crlf_newlines,
            &lines
        );
        HM_TEST_ASSERT_OK_OR_OOM(err);
        is_lines_initialized = HM_TRUE;
        HM_TEST_ASSERT(hmArrayGetCount(&lines) == line_count);
        hmString* raw = hmArrayGetRaw(&lines, hmString);
        for (hm_nint i = 0; i < line_count; i++) {
            HM_TEST_ASSERT(hmStringEqualsToCString(&raw[i], line_reader_lines[i % (sizeof(line_reader_lines) / sizeof(char*))]));
        }
    HM_TEST_ON_FINALIZE
        err = hmReaderClose(&memory_reader);
        HM_TEST_ASSERT_OK(err);
        err = hmStringDispose(&content);
        HM_TEST_ASSERT_OK(err);
        if (is_lines_initialized) {
            err = hmArrayDispose(&lines);
            HM_TEST_ASSERT_OK(err);
        }
        HM_TEST_DEINIT_ALLOC(&allocator);
    }
}

static void test_line_reader_can_read_several_lines()
{
    /* Tests with different buffer sizes and content lengths. */
    for (hm_nint i = 1; i < LINE_READER_BUFFER_SIZE; i++) {
        for (hm_nint j = 0; j < LINE_READER_MAX_LINE_COUNT; j++) {
            test_line_reader_can_read_several_lines_impl(i, j);
        }
    }
}

static void test_line_reader_ignores_trailing_new_line()
{
    hmAllocator allocator;
    hmLineReader line_reader;
    char buffer[LINE_READER_BUFFER_SIZE];
    create_line_reader_and_allocator(&line_reader, &allocator, "Hello, World!\n\n", buffer, sizeof(buffer), HM_FALSE);
    hmString string1, string2, string3;
    hm_bool is_string1_initialized = HM_FALSE,
            is_string2_initialized = HM_FALSE,
            is_string3_initialized = HM_FALSE;
    hmError err = hmLineReaderReadLine(&line_reader, &string1);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string1_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string1, "Hello, World!"));
    err = hmLineReaderReadLine(&line_reader, &string2);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string2_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string2, ""));
    err = hmLineReaderReadLine(&line_reader, &string3);
    HM_TEST_ASSERT_ERROR_OR_OOM(err, HM_ERROR_INVALID_STATE);
    if (err == HM_OK) {
        is_string3_initialized = HM_TRUE;
    }
HM_TEST_ON_FINALIZE
    if (is_string1_initialized) {
        err = hmStringDispose(&string1);
        HM_TEST_ASSERT_OK(err);
    }
    if (is_string2_initialized) {
        err = hmStringDispose(&string2);
        HM_TEST_ASSERT_OK(err);
    }
    if (is_string3_initialized) {
        err = hmStringDispose(&string3);
        HM_TEST_ASSERT_OK(err);
    }
    dispose_line_reader_and_allocator(&line_reader, &allocator);
}

static void test_line_reader_expects_empty_reader()
{
    hmAllocator allocator;
    hmLineReader line_reader;
    char buffer[LINE_READER_BUFFER_SIZE];
    create_line_reader_and_allocator(&line_reader, &allocator, "", buffer, sizeof(buffer), HM_FALSE);
    hmString string;
    hmError err = hmLineReaderReadLine(&line_reader, &string);
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_STATE);
    dispose_line_reader_and_allocator(&line_reader, &allocator);
}

static hmError hmFailingReader_read(hmReader* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read)
{
    return HM_ERROR_PLATFORM_DEPENDENT;
}

static hmError hmFailingReader_seek(hmReader* reader, hm_nint offset)
{
    return HM_ERROR_PLATFORM_DEPENDENT;
}

static hmError hmFailingReader_close(hmReader* reader)
{
    return HM_ERROR_PLATFORM_DEPENDENT;
}

static hmError hmCreateFailingReader(hmReader* in_reader)
{
    in_reader->read = &hmFailingReader_read;
    in_reader->seek = &hmFailingReader_seek;
    in_reader->close = &hmFailingReader_close;
    in_reader->data = HM_NULL;
    return HM_OK;
}

static void test_line_reader_propagates_errors_from_source_reader()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmReader reader;
    err = hmCreateFailingReader(&reader);
    HM_TEST_ASSERT_OK(err);
    char buffer[LINE_READER_BUFFER_SIZE];
    hmArray lines;
    err = hmReadAllLines(
        &allocator,
        reader,
        buffer,
        sizeof(buffer),
        HM_FALSE, /* has_crlf_newlines = HM_FALSE */
        &lines
    );
    HM_TEST_ASSERT(err == HM_ERROR_PLATFORM_DEPENDENT);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

static void test_line_reader_with_crlf_newlines_doesnt_treat_lf_as_newlines()
{
    hmAllocator allocator;
    hmLineReader line_reader;
    char buffer[LINE_READER_BUFFER_SIZE];
    create_line_reader_and_allocator(&line_reader, &allocator, "Hello,\nWorld!\r\nGoodbye,\nWorld!\r\n", buffer, sizeof(buffer), HM_TRUE);
    hmString string1, string2;
    hm_bool is_string1_initialized = HM_FALSE,
            is_string2_initialized = HM_FALSE;
    hmError err = hmLineReaderReadLine(&line_reader, &string1);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string1_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string1, "Hello,\nWorld!"));
    err = hmLineReaderReadLine(&line_reader, &string2);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string2_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string2, "Goodbye,\nWorld!"));
HM_TEST_ON_FINALIZE
    if (is_string1_initialized) {
        err = hmStringDispose(&string1);
        HM_TEST_ASSERT_OK(err);
    }
    if (is_string2_initialized) {
        err = hmStringDispose(&string2);
        HM_TEST_ASSERT_OK(err);
    }
    dispose_line_reader_and_allocator(&line_reader, &allocator);
}

static void test_line_reader_with_lf_newlines_doesnt_treat_crlf_as_newlines()

{
    hmAllocator allocator;
    hmLineReader line_reader;
    char buffer[LINE_READER_BUFFER_SIZE];
    create_line_reader_and_allocator(&line_reader, &allocator, "Hello,\nWorld!\r\nGoodbye", buffer, sizeof(buffer), HM_FALSE);
    hmString string1, string2, string3;
    hm_bool is_string1_initialized = HM_FALSE,
            is_string2_initialized = HM_FALSE,
            is_string3_initialized = HM_FALSE;
    hmError err = hmLineReaderReadLine(&line_reader, &string1);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string1_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string1, "Hello,"));
    err = hmLineReaderReadLine(&line_reader, &string2);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string2_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string2, "World!\r"));
    err = hmLineReaderReadLine(&line_reader, &string3);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string3_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string3, "Goodbye"));
HM_TEST_ON_FINALIZE
    if (is_string1_initialized) {
        err = hmStringDispose(&string1);
        HM_TEST_ASSERT_OK(err);
    }
    if (is_string2_initialized) {
        err = hmStringDispose(&string2);
        HM_TEST_ASSERT_OK(err);
    }
    if (is_string3_initialized) {
        err = hmStringDispose(&string3);
        HM_TEST_ASSERT_OK(err);
    }
    dispose_line_reader_and_allocator(&line_reader, &allocator);
}

static void test_line_readers_crlf_newline_can_straddle_two_buffer_reads()
{
    hmAllocator allocator;
    hmLineReader line_reader;
    char buffer[4]; /*  */
    create_line_reader_and_allocator(&line_reader, &allocator, "123\r\n456", buffer, sizeof(buffer), HM_TRUE);
    hmString string1, string2;
    hm_bool is_string1_initialized = HM_FALSE,
            is_string2_initialized = HM_FALSE;
    hmError err = hmLineReaderReadLine(&line_reader, &string1);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string1_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string1, "123"));
    err = hmLineReaderReadLine(&line_reader, &string2);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_string2_initialized = HM_TRUE;
    HM_TEST_ASSERT(hmStringEqualsToCString(&string2, "456"));
HM_TEST_ON_FINALIZE
    if (is_string1_initialized) {
        err = hmStringDispose(&string1);
        HM_TEST_ASSERT_OK(err);
    }
    if (is_string2_initialized) {
        err = hmStringDispose(&string2);
        HM_TEST_ASSERT_OK(err);
    }
    dispose_line_reader_and_allocator(&line_reader, &allocator);
}

HM_TEST_SUITE_BEGIN(line_readers)
    HM_TEST_RUN(test_line_reader_supports_never_being_read)
    HM_TEST_RUN(test_line_reader_can_read_several_lines)
    HM_TEST_RUN(test_line_reader_ignores_trailing_new_line)
    HM_TEST_RUN(test_line_reader_expects_empty_reader)
    HM_TEST_RUN(test_line_reader_propagates_errors_from_source_reader)
    HM_TEST_RUN(test_line_reader_with_crlf_newlines_doesnt_treat_lf_as_newlines)
    HM_TEST_RUN(test_line_reader_with_lf_newlines_doesnt_treat_crlf_as_newlines)
    HM_TEST_RUN(test_line_readers_crlf_newline_can_straddle_two_buffer_reads)
HM_TEST_SUITE_END()
