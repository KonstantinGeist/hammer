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

#ifndef HM_LINE_READER_H
#define HM_LINE_READER_H

#include <core/string.h>
#include <core/stringbuilder.h>
#include <io/reader.h>

typedef struct {
    hmReader        source_reader;
    hmStringBuilder string_builder;
    hmAllocator*    allocator;
    char*           buffer;
    hm_nint         buffer_size;
    hm_nint         buffer_index;
    hm_bool         close_source_reader;
} hmLineReader;

hmError hmCreateLineReader(
    hmAllocator*  allocator,
    hmReader      source_reader,
    hm_bool       close_source_reader,
    char*         buffer,
    hm_nint       buffer_size,
    hmLineReader* in_line_reader
);
hmError hmLineReaderDispose(hmLineReader* line_reader);
hmError hmLineReaderReadLine(hmLineReader* line_reader, hmString* in_line);

#endif /* HM_LINE_READER_H */
