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

#ifndef HM_WRITER_H
#define HM_WRITER_H

#include <core/common.h>
#include <core/allocator.h>
#include <core/string.h>

#define HM_WRITER_DEFAULT_BUFFER_SIZE (4*1024) /* 4KB */

/* Generic structure for any writer. Writers can be used to write to any medium: memory, sockets, files on disk, etc. */
typedef struct hmWriter_ {
    hmError (*write)(struct hmWriter_* writer, const char* buffer, hm_nint size, hm_nint* out_bytes_written); /* Reads `size` number of bytes to `buffer`, returns `out_bytes_written`. */
    hmError (*close)(struct hmWriter_* writer);
    void*     data;                                            /* Writer-specific data. */
} hmWriter;

/* Writes `size` number of bytes from `buffer`, returns `out_bytes_written`. */
hmError hmWriterWrite(hmWriter* writer, const char* buffer, hm_nint size, hm_nint* out_bytes_written);
/* Closes the writer, freeing all additional resources. */
hmError hmWriterClose(hmWriter *writer);

/* Creates a string writer which writes to a string. Call hmStringWriterGetString(..) to retrieve the string. */
hmError hmCreateStringWriter(hmAllocator* allocator, hmWriter* in_writer);
/* Retrieves the string which is created by write calls to the string writer. Each time a new copy is returned.
  `allocator_opt` specifies the allocator with which to allocate the string. If no allocator is provided, the string writer's
   allocator is reused.
   The behavior is undefined if `writer` is not a string writer. */
hmError hmStringWriterGetString(hmWriter* writer, hmAllocator* allocator_opt, hmString* in_string);

#endif /* HM_WRITER_H */
