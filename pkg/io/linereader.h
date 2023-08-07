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
    hm_bool         has_more_lines;      /* Becomes HM_FALSE the first time `source_reader` returns 0 read bytes. */
    hm_bool         close_source_reader; /* If true, the source reader will be closed when the line reader is disposed. */
    hm_bool         has_crlf_newlines;   /* Tells if newlines should be treated as CRLF ("\r\n") instead of LF ("\n"). */
} hmLineReader;

/* A line reader takes a `source_reader` and progressively reads lines separated by newlines from it via hmLineReaderReadLine(..) (see).
   If `close_source_reader` is true, `source_reader` is automatically closed when the line reader is disposed.
  `buffer` and `buffer_size` specify the internal scratch buffer which will be used. Useful for tests and to control memory usage.
   If `has_crlf_newlines` is set to HM_TRUE, treats newlines as CRLF ("\r\n") instead of LF ("\n"). For example, the HTTP protocol
   supports only CRLF newlines. */
hmError hmCreateLineReader(
    hmAllocator*  allocator,
    hmReader      source_reader,
    hm_bool       close_source_reader,
    char*         buffer,
    hm_nint       buffer_size,
    hm_bool       has_crlf_newlines,
    hmLineReader* in_line_reader
);
/* Disposes of the line reader. If `close_source_reader` is true, also closes the source reader specified in the
   constructor (see). */
hmError hmLineReaderDispose(hmLineReader* line_reader);
/* Reads a new line from the source reader specified in the line reader's constructor as `source_reader`.
   Reading is buffered, with the scratch memory and the buffer size specified as `buffer` and `buffer_size` in the
   constructor. Lines should be separated by "\n".
   When there are no more lines in the source reader, returns HM_ERROR_INVALID_STATE (by analogy with queues etc.)
   All reading errors from the underlying source reader are simply propagated.
   NOTE If the stream ends with a trailing newline (for example, "Hello World\n"), no empty line is returned. */
hmError hmLineReaderReadLine(hmLineReader* line_reader, hmString* in_line);
/* The line reader can "overshoot", i.e. while reading the next line from the source reader, it can read more bytes
   than necessary for the next line, because it reads in fixed size chunks. This function returns what's left in the buffer by
   placing the pointer to the internal buffer in `out_buffer` with `out_size`. The returned buffer is valid as long as
   the line reader is valid: copy it to a different buffer if you want it to survive a call to hmLineReaderDispose(..)
   The function is useful when the source reader is shared between multiple clients: for example, one client (i.e. hmLineReader)
   wants to read several lines up to some point, and another client wants to start reading where hmLineReader left off.
   Subsequent calls to hmLineReaderReadLine(..) can change the contents of the buffer. */
hmError hmLineReaderGetBuffered(hmLineReader* line_reader, char** out_buffer, hm_nint* out_size);

/* A helper function which creates a temporary line reader from the given `reader`, reads all lines, accumulates them
   in an array, and then disposes of the temporary line reader.
   For the arguments and behavior, see hmCreateLineReader(..) and hmLineReaderReadLine(..)
   NOTE `reader` is never automatically closed by this function. */
hmError hmReadAllLines(
    hmAllocator* allocator,
    hmReader     reader,
    char*        buffer,
    hm_nint      buffer_size,
    hm_bool      has_crlf_newlines,
    hmArray*     in_array
);

#endif /* HM_LINE_READER_H */
