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

#include <io/reader.h>
#include <core/allocator.h>
#include <core/math.h>
#include <core/utils.h>

hmError hmReaderRead(hmReader* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read)
{
    return reader->read(reader, buffer, size, out_bytes_read);
}

hmError hmReaderSeek(hmReader* reader, hm_nint size)
{
    return reader->seek(reader, size);
}

hmError hmReaderClose(hmReader* reader)
{
    return reader->close(reader);
}

/* ******************* */
/*    MemoryReader.    */
/* ******************* */

typedef struct {
    const char*  base;      /* The original pointer to the beginning of the memory block. */
    hmAllocator* allocator; /* The allocator which governs this structure's lifetime. */
    hm_nint      offset;    /* The current pointer/offset. */
    hm_nint      size;      /* The size of the memory block. */
} hmMemoryReaderData;

static hmError hmMemoryReader_read(hmReader* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read)
{
    if (!buffer || !out_bytes_read) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    if (!size) {
        *out_bytes_read = 0;
        return HM_OK; /* do nothing because we were told to read 0 bytes */
    }
    hmMemoryReaderData* data = (hmMemoryReaderData*)reader->data;
    hm_nint bytes_read = size;
    hm_nint offset_with_size = 0;
    HM_TRY(hmAddNint(data->offset, size, &offset_with_size));
    if (offset_with_size > data->size) {
        bytes_read = data->size - data->offset;
    }
    hm_nint base_with_offset = 0;
    HM_TRY(hmAddNint(hmCastPointerToNint(data->base), data->offset, &base_with_offset));
    hmCopyMemory(buffer, hmCastNintToPointer(base_with_offset, const char*), bytes_read);
    data->offset = offset_with_size;
    *out_bytes_read = bytes_read;
    return HM_OK;
}

static hmError hmMemoryReader_seek(hmReader* reader, hm_nint offset)
{
    hmMemoryReaderData* data = (hmMemoryReaderData*)reader->data;
    if (offset >= data->size) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    data->offset = offset;
    return HM_OK;
}

static hmError hmMemoryReader_close(hmReader* reader)
{
    hmMemoryReaderData* data = (hmMemoryReaderData*)reader->data;
    hmFree(data->allocator, (char*)reader->data);
    return HM_OK;
}

hmError hmCreateMemoryReader(hmAllocator* allocator, const char* mem, hm_nint mem_size, hmReader* in_reader)
{
    if (!mem || !mem_size || !allocator || !in_reader) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmMemoryReaderData* data = hmAlloc(allocator, sizeof(hmMemoryReaderData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    data->base = mem;
    data->allocator = allocator;
    data->offset = 0;
    data->size = mem_size;
    in_reader->read = &hmMemoryReader_read;
    in_reader->seek = &hmMemoryReader_seek;
    in_reader->close = &hmMemoryReader_close;
    in_reader->data = data;
    return HM_OK;
}
