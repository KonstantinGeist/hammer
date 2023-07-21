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
#include <collections/array.h>
#include <io/reader.h>

typedef struct {
    hmReader        source_reader;       /* The source reader. */
    hmStringBuilder next_line_builder;   /* Used to build the next line if it spans several buffered reading calls. */
    hmAllocator*    allocator;
    char*           buffer;              /* Scratch memory for buffered reading. */
    hm_nint         buffer_size;         /* The size of the scratch memory for buffered reading. */
    hm_nint         buffer_index;        /* The current index inside buffered data when scanning the buffer for newlines. */
    hm_nint         bytes_read;          /* The number of read bytes can be less than `buffer_size`, so we remember that. */
    hm_bool         has_more_lines;      /* Becomes HM_TRUE the first time `source_reader` returns 0 read bytes. */
    hm_bool         close_source_reader; /* If true, the source reader will be closed when the line reader is disposed. */
} hmLineReader;

/* A line reader takes a `source_reader` and progressively reads lines separated by newlines from it via hmLineReaderReadLine(..) (see).
   If `close_source_reader` is true, `source_reader` is automatically closed when the line reader is disposed.
  `buffer` and `buffer_size` specify the internal scratch buffer which will be used. Useful for tests and to control memory usage. */
hmError hmCreateLineReader(
    hmAllocator*  allocator,
    hmReader      source_reader,
    hm_bool       close_source_reader,
    char*         buffer,
    hm_nint       buffer_size,
    hmLineReader* in_line_reader
);
/* Disposes of the line reader. If `close_source_reader` is true, also closes the source reader specified in the
   constructor (see). */
hmError hmLineReaderDispose(hmLineReader* line_reader);
/* Reads a new line from the source reader specified in the line reader's constructor as `source_reader`.
   Reading is buffered, with the scratch memory and the buffer size specified as `buffer` and `buffer_size` in the
   constructor. Lines should be separated by "\n".
   When there are no more lines in the source reader, returns HM_ERROR_INVALID_STATE (by analogy with queues etc.)
   All reading errors from the underlying source reader are simply propagated. */
hmError hmLineReaderReadLine(hmLineReader* line_reader, hmString* in_line);

/* A helper function which creates a temporary line reader from the given `reader`, reads all lines, accumulates them
   in an array, and then disposes of the temporary line reader.
   For the arguments and behavior, see hmCreateLineReader(..) and hmLineReaderReadLine(..)
   Note: `reader` is never automatically closed by this function. */
hmError hmReadAllLines(
    hmAllocator* allocator,
    hmReader     reader,
    char*        buffer,
    hm_nint      buffer_size,
    hmArray*     in_array
);

#endif /* HM_LINE_READER_H */
