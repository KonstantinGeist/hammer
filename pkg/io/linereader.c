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

#include <io/linereader.h>

hmError hmCreateLineReader(
    hmAllocator*  allocator,
    hmReader      source_reader,
    hm_bool       close_source_reader,
    char*         buffer,
    hm_nint       buffer_size,
    hmLineReader* in_line_reader
)
{
    if (!buffer_size) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    HM_TRY(hmCreateStringBuilder(allocator, &in_line_reader->string_builder));
    in_line_reader->source_reader = source_reader;
    in_line_reader->allocator = allocator;
    in_line_reader->buffer = buffer;
    in_line_reader->buffer_size = buffer_size;
    in_line_reader->buffer_index = 0;
    in_line_reader->bytes_read = 0;
    in_line_reader->has_more_lines = HM_TRUE;
    in_line_reader->close_source_reader = close_source_reader;
    return HM_OK;
}

hmError hmLineReaderDispose(hmLineReader* line_reader)
{
    hmError err = hmStringBuilderDispose(&line_reader->string_builder);
    if (line_reader->close_source_reader) {
        err = hmMergeErrors(err, hmReaderClose(&line_reader->source_reader));
    }
    return err;
}

hmError hmLineReaderReadLine(hmLineReader* line_reader, hmString* in_line)
{
    if (!line_reader->has_more_lines) {
        return HM_ERROR_INVALID_STATE;
    }
again: {}
    hmError err = HM_OK;
    if (line_reader->bytes_read == 0) {
        hm_nint bytes_read;
        HM_TRY(hmReaderRead(&line_reader->source_reader, line_reader->buffer, line_reader->buffer_size, &bytes_read));
        /* To avoid buffer overflows, as we don't know if the underlying reader behaves correctly. */
        if (bytes_read > line_reader->buffer_size) {
            return HM_ERROR_OVERFLOW;
        }
        if (bytes_read == 0) {
            line_reader->has_more_lines = HM_FALSE;
            if (hmStringBuilderGetLength(&line_reader->string_builder) == 0) {
                return HM_ERROR_INVALID_STATE;
            }
            HM_TRY(hmStringBuilderAppendCStringWithLength(
                &line_reader->string_builder,
                line_reader->buffer + line_reader->buffer_index,
                line_reader->bytes_read - line_reader->buffer_index
            ));
            err = hmStringBuilderToString(&line_reader->string_builder, HM_NULL, in_line);
            hm_bool is_string_initialized = err == HM_OK;
            err = hmMergeErrors(err, hmStringBuilderClear(&line_reader->string_builder));
            if (err != HM_OK) {
                if (is_string_initialized) {
                    err = hmMergeErrors(err, hmStringDispose(in_line));
                }
            }
            return err;
        }
        line_reader->bytes_read = bytes_read;
    }
    for (hm_nint i = line_reader->buffer_index; i < line_reader->bytes_read; i++) {
        if (line_reader->buffer[i] == '\n') {  /* TODO iterate as UTF8 code points */
            /* No safe math for `line_reader->buffer + line_reader->buffer_index` because the resulting value is
                bounded to [buffer, buffer + buffer_size). */
            HM_TRY(hmStringBuilderAppendCStringWithLength(
                &line_reader->string_builder,
                line_reader->buffer + line_reader->buffer_index,
                i - line_reader->buffer_index
            ));
            err = hmStringBuilderToString(&line_reader->string_builder, HM_NULL, in_line);
            hm_bool is_string_initialized = err == HM_OK;
            err = hmMergeErrors(err, hmStringBuilderClear(&line_reader->string_builder));
            if (err != HM_OK) {
                if (is_string_initialized) {
                    err = hmMergeErrors(err, hmStringDispose(in_line));
                }
                return err;
            }
            line_reader->buffer_index = i + 1; /* no safe math, because `i + 1` is bounded to [0, bytes_read] */
            return HM_OK;
        }
    }
    /* The buffer is fully scanned and there's no more newlines found, so append the reminder to the string builder
        so that the reminder was picked up by the next call to hmLineReaderReadLine(..) */
    HM_TRY(hmStringBuilderAppendCStringWithLength(
        &line_reader->string_builder,
        line_reader->buffer + line_reader->buffer_index,
        line_reader->bytes_read - line_reader->buffer_index
    ));
    line_reader->buffer_index = 0; /* rewinds the buffer's index back to the beginning */
    line_reader->bytes_read = 0;
    goto again;
    return err;
}

hmError hmReadAllLines(
    hmAllocator* allocator,
    hmReader     reader,
    char*        buffer,
    hm_nint      buffer_size,
    hmArray*     in_array
)
{
    hmLineReader line_reader;
    HM_TRY(hmCreateLineReader(
        allocator,
        reader,
        HM_FALSE, /* close_source_reader = HM_FALSE */
        buffer,
        buffer_size,
        &line_reader
    ));
    hmError err = HM_OK;
    hm_bool is_array_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmCreateArray(
        allocator,
        sizeof(hmString),
        HM_ARRAY_DEFAULT_CAPACITY,
        &hmStringDisposeFunc,
        in_array
    ));
    is_array_initialized = HM_TRUE;
    hmString string;
    while ((err = hmLineReaderReadLine(&line_reader, &string)) == HM_OK) {
        err = hmArrayAdd(in_array, &string);
        if (err != HM_OK) {
            err = hmMergeErrors(err, hmStringDispose(&string));
            HM_FINALIZE;
        }
    }
    if (err == HM_ERROR_INVALID_STATE) {
        err = HM_OK;
    }
HM_ON_FINALIZE
    err = hmMergeErrors(err, hmLineReaderDispose(&line_reader));
    if (err != HM_OK) {
        if (is_array_initialized) {
            err = hmMergeErrors(err, hmArrayDispose(in_array));
        }
    }
    return err;
}
