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
#include <core/math.h>

static hm_bool hmLineReaderShouldReadFromSourceReader(hmLineReader* line_reader);
static void hmLineReaderScheduleMoreReadingFromSourceReader(hmLineReader* line_reader);
static hmError hmLineReaderReadFromSourceReader(hmLineReader* line_reader, hmString* in_line, hm_bool* out_is_line_formed);
static hmError hmLineReaderScanBufferForNextLine(hmLineReader* line_reader, hmString* in_line, hm_bool* out_is_line_formed);
static hmError hmLineReaderAppendRemainingInBufferToNextLine(hmLineReader* line_reader);

hmError hmCreateLineReader(
    hmAllocator*  allocator,
    hmReader      source_reader,
    hm_bool       close_source_reader,
    char*         buffer,
    hm_nint       buffer_size,
    hm_bool       has_crlf_newlines,
    hmLineReader* in_line_reader
)
{
    if (!buffer_size) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    HM_TRY(hmCreateStringBuilder(allocator, &in_line_reader->next_line_builder));
    in_line_reader->source_reader = source_reader;
    in_line_reader->allocator = allocator;
    in_line_reader->buffer = buffer;
    in_line_reader->buffer_size = buffer_size;
    in_line_reader->buffer_index = 0;
    in_line_reader->bytes_read = 0;
    in_line_reader->has_more_lines = HM_TRUE;
    in_line_reader->close_source_reader = close_source_reader;
    in_line_reader->has_crlf_newlines = has_crlf_newlines;
    return HM_OK;
}

hmError hmLineReaderDispose(hmLineReader* line_reader)
{
    hmError err = hmStringBuilderDispose(&line_reader->next_line_builder);
    if (line_reader->close_source_reader) {
        err = hmMergeErrors(err, hmReaderClose(&line_reader->source_reader));
    }
    return err;
}

hmError hmLineReaderReadLine(hmLineReader* line_reader, hmString* in_line)
{
    /* The loop is quite simple:
       - reads from the source reader into the buffer if necessary (may form the next line if it can't read from the
         source reader anymore and there's still some stuff remaining in the buffer);
       - scans the buffer for the next line (i.e. by looking for the first "\n") and forms a new line on success;
       - appends remaining stuff in the buffer to the next line if the previous scanning of the buffer for "\n" was not
         successful (i.e. the next line spans several buffered reading calls because it's large);
       - repeats until one of the functions above forms a line. */
    if (!line_reader->has_more_lines) {
        return HM_ERROR_INVALID_STATE;
    }
    while (HM_TRUE) {
        hm_bool is_line_formed = HM_FALSE;
        if (hmLineReaderShouldReadFromSourceReader(line_reader)) {
            HM_TRY(hmLineReaderReadFromSourceReader(line_reader, in_line, &is_line_formed));
            if (is_line_formed) {
                break;
            }
        }
        HM_TRY(hmLineReaderScanBufferForNextLine(line_reader, in_line, &is_line_formed));
        if (is_line_formed) {
            break;
        }
        HM_TRY(hmLineReaderAppendRemainingInBufferToNextLine(line_reader));
        hmLineReaderScheduleMoreReadingFromSourceReader(line_reader);
    }
    return HM_OK;
}

/* NOTE: similar to hmLineReaderAppendRemainingInBufferToNextLine(..) */
hmError hmLineReaderGetBuffered(hmLineReader* line_reader, char** out_buffer, hm_nint* out_size)
{
    hm_nint buffer_with_index_offset = 0, remaining_size = 0;
    HM_TRY(hmAddNint(hmCastPointerToNint(line_reader->buffer), line_reader->buffer_index, &buffer_with_index_offset));
    HM_TRY(hmSubNint(line_reader->bytes_read, line_reader->buffer_index, &remaining_size));
    *out_buffer = hmCastNintToPointer(buffer_with_index_offset, char*);
    *out_size = remaining_size;
    return HM_OK;
}

hmError hmReadAllLines(
    hmAllocator* allocator,
    hmReader     reader,
    char*        buffer,
    hm_nint      buffer_size,
    hm_bool      has_crlf_newlines,
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
        has_crlf_newlines,
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
    /* According to the specification of hmLineReaderReadLine(..), error code HM_ERROR_INVALID_STATE tells that there
       are no more lines in the line reader, so we convert it to HM_OK, because it's not actually an error as far as
       hmReadAllLines(..) is concerned. */
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

static hm_bool hmLineReaderShouldReadFromSourceReader(hmLineReader* line_reader)
{
    return line_reader->bytes_read == 0;
}

static void hmLineReaderScheduleMoreReadingFromSourceReader(hmLineReader* line_reader)
{
    line_reader->bytes_read = 0;
}

static hmError hmLineReaderAppendToNextLineBuilder(hmLineReader* line_reader, char* chars, hm_nint length_in_bytes)
{
    return hmStringBuilderAppendCStringWithLength(&line_reader->next_line_builder, chars, length_in_bytes);
}

static hmError hmLineReaderCreateLineFromNextLineBuilder(hmLineReader* line_reader, hmString* in_line)
{
    hm_nint line_length_in_bytes = hmStringBuilderGetLengthInBytes(&line_reader->next_line_builder);
    /* Support for CRLF newlines: removes "\r" before "\n" (this function accepts lines which are always split by "\n"
       whether we want CRLF or LF newlines).
       No safe math for "line_length_in_bytes - 1" and "line_length_in_bytes--" because the bounds are checked in "line_length_in_bytes > 0". */
    if (line_reader->has_crlf_newlines
        && line_length_in_bytes > 0
        && hmStringBuilderGetChars(&line_reader->next_line_builder)[line_length_in_bytes - 1] == '\r')
    {
        line_length_in_bytes--;
    }
    return hmStringBuilderToStringWithStartIndexAndLengthInBytes(&line_reader->next_line_builder, HM_NULL, 0, line_length_in_bytes, in_line);
}

static hmError hmLineReaderResetNextLineBuilder(hmLineReader* line_reader)
{
    return hmStringBuilderClear(&line_reader->next_line_builder);
}

/* See hmLineReaderReadLine(..) for the overview of the algorithm. */
static hmError hmLineReaderAppendRemainingInBufferToNextLine(hmLineReader* line_reader)
{
    hm_nint buffer_with_index_offset = 0, remaining_size = 0;
    HM_TRY(hmAddNint(hmCastPointerToNint(line_reader->buffer), line_reader->buffer_index, &buffer_with_index_offset));
    HM_TRY(hmSubNint(line_reader->bytes_read, line_reader->buffer_index, &remaining_size));
    HM_TRY(hmLineReaderAppendToNextLineBuilder(line_reader, hmCastNintToPointer(buffer_with_index_offset, char*), remaining_size));
    line_reader->buffer_index = 0;
    return HM_OK;
}

/* See hmLineReaderReadLine(..) for the overview of the algorithm. */
static hmError hmLineReaderReadFromSourceReader(hmLineReader* line_reader, hmString* in_line, hm_bool* out_is_line_formed)
{
    *out_is_line_formed = HM_FALSE;
    hm_nint bytes_read = 0;
    HM_TRY(hmReaderRead(&line_reader->source_reader, line_reader->buffer, line_reader->buffer_size, &bytes_read));
    /* A check to avoid buffer overflows, as we don't know if the underlying reader behaves correctly. */
    if (bytes_read > line_reader->buffer_size) {
        return HM_ERROR_OVERFLOW;
    }
    if (bytes_read == 0) { /* can't read from the buffer anymore => reached the end of the stream */
        line_reader->has_more_lines = HM_FALSE;
        if (hmStringBuilderGetLengthInBytes(&line_reader->next_line_builder) == 0) { /* no more lines => the end */
            return HM_ERROR_INVALID_STATE;
        }
        /* We can't read from the source reader anymore but there's some stuff still found in the buffer => form it as the
           next (and last) line. */
        hm_nint buffer_with_index_offset = 0, remaining_size = 0;
        HM_TRY(hmAddNint(hmCastPointerToNint(line_reader->buffer), line_reader->buffer_index, &buffer_with_index_offset));
        HM_TRY(hmSubNint(line_reader->bytes_read, line_reader->buffer_index, &remaining_size));
        HM_TRY(hmLineReaderAppendToNextLineBuilder(line_reader, hmCastNintToPointer(buffer_with_index_offset, char*), remaining_size));
        hmError err = hmLineReaderCreateLineFromNextLineBuilder(line_reader, in_line);
        hm_bool is_string_initialized = err == HM_OK;
        err = hmMergeErrors(err, hmLineReaderResetNextLineBuilder(line_reader));
        if (err != HM_OK) {
            if (is_string_initialized) {
                err = hmMergeErrors(err, hmStringDispose(in_line));
            }
        } else {
            *out_is_line_formed = HM_TRUE;
        }
        return err;
    }
    line_reader->bytes_read = bytes_read;
    return HM_OK;
}

static hm_bool hmLineReaderIsNewline(hmLineReader* line_reader, hm_nint index)
{
    /* NOTE: it's safe to look for '\n' in a UTF8 string as it's guaranteed to not be part of any non-ASCII code point by design. */
    if (line_reader->has_crlf_newlines) {
        if (line_reader->buffer[index] != '\n') {
            return HM_FALSE;
        }
        /* Looks for the preceding "\r" in the buffer as there's some space in it before "\n". */
        if (index > line_reader->buffer_index) {
            return line_reader->buffer[index - 1] == '\r'; /* no safe math because "index" can't be 0 after "index > line_reader->buffer_index" */
        }
        /* Checks for "\r" in the next line builder. */
        hm_nint next_line_builder_length_in_bytes = hmStringBuilderGetLengthInBytes(&line_reader->next_line_builder);
        if (next_line_builder_length_in_bytes == 0) { /* it's empty => not a CLRF newline */
            return HM_FALSE;
        }
        /* No safe math for "next_line_builder_length_in_bytes - 1" because we checked above that "next_line_builder_length_in_bytes > 0" */
        return hmStringBuilderGetChars(&line_reader->next_line_builder)[next_line_builder_length_in_bytes - 1] == '\r';
    } else {
        return line_reader->buffer[index] == '\n';
    }
}

/* See hmLineReaderReadLine(..) for the overview of the algorithm. */
static hmError hmLineReaderScanBufferForNextLine(hmLineReader* line_reader, hmString* in_line, hm_bool* out_is_line_formed)
{
    *out_is_line_formed = HM_FALSE;
    for (hm_nint i = line_reader->buffer_index; i < line_reader->bytes_read; i++) {
        if (hmLineReaderIsNewline(line_reader, i)) {
            hm_nint buffer_with_index_offset = 0, remaining_size = 0;
            HM_TRY(hmAddNint(hmCastPointerToNint(line_reader->buffer), line_reader->buffer_index, &buffer_with_index_offset));
            HM_TRY(hmSubNint(i, line_reader->buffer_index, &remaining_size));
            HM_TRY(hmLineReaderAppendToNextLineBuilder(line_reader, hmCastNintToPointer(buffer_with_index_offset, char*), remaining_size));
            hmError err = hmLineReaderCreateLineFromNextLineBuilder(line_reader, in_line);
            hm_bool is_string_initialized = err == HM_OK;
            err = hmMergeErrors(err, hmLineReaderResetNextLineBuilder(line_reader));
            if (err == HM_OK) {
                err = hmMergeErrors(err, hmAddNint(i, 1, &line_reader->buffer_index));
            } else {
                if (is_string_initialized) {
                    err = hmMergeErrors(err, hmStringDispose(in_line));
                }
                return err;
            }
            *out_is_line_formed = HM_TRUE;
            return HM_OK;
        }
    }
    return HM_OK;
}
