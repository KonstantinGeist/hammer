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

#ifndef HM_READER_H
#define HM_READER_H

#include <core/common.h>
#include <core/allocator.h>

#define HM_READER_DEFAULT_BUFFER_SIZE (4*1024) /* 4KB */

/* Generic structure for any reader. Readers can be used to read runtime metadata from disk, memory, etc. */
typedef struct hmReader_ {
    hmError (*read)(struct hmReader_* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read); /* Reads `size` number of bytes to `buffer`, returns `out_bytes_read`. */
    hmError (*close)(struct hmReader_* reader);
    void*     data;                                            /* Reader-specific data. */
} hmReader;

/* Reads `size` number of bytes to `buffer`, returns `out_bytes_read`. If `out_bytes_read` is 0, it means there's no
   more data in the reader. */
hmError hmReaderRead(hmReader* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read);
/* Closes the reader, freeing all additional resources. */
hmError hmReaderClose(hmReader *reader);

/* Creates a reader which reads from a given fixed memory block and initialized data pointed to by in_reader.
   Useful when data is constructed in-memory; for example, in tests. */
hmError hmCreateMemoryReader(hmAllocator* allocator, const char* mem, hm_nint mem_size, hmReader* in_reader);
/* Gets the current position of the memory reader. Useful for tests.
   The behavior is undefined if `reader` is not a memory reader. */
hm_nint hmMemoryReaderGetPosition(hmReader* reader);
/* Sets the current position of the memory reader. Useful for tests.
   The behavior is undefined if `reader` is not a memory reader. */
hmError hmMemoryReaderSetPosition(hmReader* reader, hm_nint offset);
/* Creates a limited reader which wraps another reader `source_reader` and returns HM_ERROR_LIMIT_EXCEEDED if more than
   `limit_in_bytes` bytes is read from `source_reader`. Useful when limiting the amount of data to be read, for example
   in the web context. If `close_source_reader` is set to true, the limited reader closes the buffer it wraps when it's
   closed itself. */
hmError hmCreateLimitedReader(
   hmAllocator* allocator,
   hmReader     source_reader,
   hm_bool      close_source_reader,
   hm_nint      limit_in_bytes,
   hmReader*    in_reader
);
/* A composite reader represents several readers as a single reader:
   - reads from the first reader until there's no more data in it;
   - then reads from the second reader until there's no more data in it;
   - etc.
   `source_readers` is a list of readers to wrap, with `source_reader_count` specifying their count.
   `close_source_readers` specifies, for each reader in `source_readers` at the corresponding indices, whether the source
   readers should be closed when the composite reader closes. */
hmError hmCreateCompositeReader(
   hmAllocator*    allocator,
   const hmReader* source_readers,
   const hm_bool*  close_source_readers,
   hm_nint         source_reader_count,
   hmReader*       in_reader
);

#endif /* HM_READER_H */
