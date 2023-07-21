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
    return HM_OK;
}
