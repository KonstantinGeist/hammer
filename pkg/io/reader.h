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

/* Generic structure for any reader. Readers can be used to read runtime metadata from disk, memory, etc. */
typedef struct hmReader_ {
    hmError (*read)(struct hmReader_* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read); /* Reads `size` number of bytes to `buffer`, returns `out_bytes_read`. */
    hmError (*seek)(struct hmReader_* reader, hm_nint offset); /* Moves the file pointer to a specific offset denoted by `offset`. */
    hmError (*close)(struct hmReader_* reader);
    void*     data;                                            /* Reader-specific data. */
} hmReader;

/* Reads `size` number of bytes to `buffer`, returns `out_bytes_read`. */
hmError hmReaderRead(hmReader* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read);
/* Moves the file pointer to a specific offset denoted by `offset`. */
hmError hmReaderSeek(hmReader* reader, hm_nint offset);
/* Closes the reader, freeing all additional resources. */
hmError hmReaderClose(hmReader *reader);

/* Creates a reader which reads from a given fixed memory block and initialized data pointed to by in_reader.
   Useful when, for example, runtime metadata is constructed in-memory. */
hmError hmCreateMemoryReader(hmAllocator* allocator, const char* mem, hm_nint mem_size, hmReader* in_reader);

#endif /* HM_READER_H */
