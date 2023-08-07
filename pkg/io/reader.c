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
    if (!size) {
        *out_bytes_read = 0;
        return HM_OK; /* do nothing because we were told to read 0 bytes */
    }
    hmMemoryReaderData* data = (hmMemoryReaderData*)reader->data;
    hm_nint bytes_read = size;
    hm_nint offset_with_size = 0;
    HM_TRY(hmAddNint(data->offset, size, &offset_with_size));
    if (offset_with_size > data->size) { /* truncates if there's an attempt to read past the buffer */
        HM_TRY(hmSubNint(data->size, data->offset, &bytes_read));
    }
    hm_nint base_with_offset = 0;
    HM_TRY(hmAddNint(hmCastPointerToNint(data->base), data->offset, &base_with_offset));
    hmCopyMemory(buffer, hmCastNintToPointer(base_with_offset, const char*), bytes_read);
    data->offset = offset_with_size > data->size ? data->size : offset_with_size;
    *out_bytes_read = bytes_read;
    return HM_OK;
}

static hmError hmMemoryReader_close(hmReader* reader)
{
    hmMemoryReaderData* data = (hmMemoryReaderData*)reader->data;
    hmFree(data->allocator, data);
    return HM_OK;
}

hmError hmCreateMemoryReader(hmAllocator* allocator, const char* mem, hm_nint mem_size, hmReader* in_reader)
{
    hmMemoryReaderData* data = hmAlloc(allocator, sizeof(hmMemoryReaderData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    data->base = mem;
    data->allocator = allocator;
    data->offset = 0;
    data->size = mem_size;
    in_reader->read = &hmMemoryReader_read;
    in_reader->close = &hmMemoryReader_close;
    in_reader->data = data;
    return HM_OK;
}

hm_nint hmMemoryReaderGetPosition(hmReader* reader)
{
    hmMemoryReaderData* data = (hmMemoryReaderData*)reader->data;
    return data->offset;
}

hmError hmMemoryReaderSetPosition(hmReader* reader, hm_nint offset)
{
    hmMemoryReaderData* data = (hmMemoryReaderData*)reader->data;
    if (offset >= data->size) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    data->offset = offset;
    return HM_OK;
}

/* ******************* */
/*    LimitedReader.   */
/* ******************* */

typedef struct {
    hmAllocator* allocator;           /* The allocator which governs this structure's lifetime. */
    hmReader     source_reader;       /* The source reader. */
    hm_nint      limit_in_bytes;      /* The Read(..) operation should return HM_ERROR_LIMIT_EXCEEDED if the number of read bytes exceeds this value. */
    hm_nint      total_bytes_read;    /* The amount of bytes read so far. Compared to `limit_in_bytes. */
    hm_bool      close_source_reader; /* If true, closes the source reader automatically when the limited reader is closed. */
} hmLimitedReaderData;

static hmError hmLimitedReader_read(hmReader* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read)
{
    if (!size) {
        *out_bytes_read = 0;
        return HM_OK; /* do nothing because we were told to read 0 bytes */
    }
    hmLimitedReaderData* data = (hmLimitedReaderData*)reader->data;
    if (data->total_bytes_read >= data->limit_in_bytes) { /* immediately fail if we hit the limit in a previous call; ">=" instead of "==" is just in case */
        return HM_ERROR_LIMIT_EXCEEDED;
    }
    hm_nint remaning_bytes_to_read = 0;
    HM_TRY(hmSubNint(data->limit_in_bytes, data->total_bytes_read, &remaning_bytes_to_read));
    if (remaning_bytes_to_read < size) {
        size = remaning_bytes_to_read;
    }
    HM_TRY(hmReaderRead(&data->source_reader, buffer, size, out_bytes_read));
    if (*out_bytes_read > size) { /* ill-behaving source reader */
        return HM_ERROR_INVALID_STATE;
    }
    HM_TRY(hmAddNint(data->total_bytes_read, *out_bytes_read, &data->total_bytes_read));
    if (data->total_bytes_read >= data->limit_in_bytes) { /* we just exceeded the limit; ">=" instead of "==" is just in case */
        return HM_ERROR_LIMIT_EXCEEDED;
    }
    return HM_OK;
}

static hmError hmLimitedReader_close(hmReader* reader)
{
    hmLimitedReaderData* data = (hmLimitedReaderData*)reader->data;
    hmError err = HM_OK;
    if (data->close_source_reader) {
        err = hmReaderClose(&data->source_reader);
    }
    hmFree(data->allocator, data);
    return err;
}

hmError hmCreateLimitedReader(
   hmAllocator* allocator,
   hmReader     source_reader,
   hm_bool      close_source_reader,
   hm_nint      limit_in_bytes,
   hmReader*    in_reader
)
{
    hmLimitedReaderData* data = hmAlloc(allocator, sizeof(hmLimitedReaderData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    data->allocator = allocator;
    data->source_reader = source_reader;
    data->close_source_reader = close_source_reader;
    data->limit_in_bytes = limit_in_bytes;
    data->total_bytes_read = 0;
    in_reader->read = &hmLimitedReader_read;
    in_reader->close = &hmLimitedReader_close;
    in_reader->data = data;
    return HM_OK;
}

/* ********************* */
/*    CompositeReader.   */
/* ********************* */

typedef struct {
    hmReader reader;       /* The wrapped reader. */
    hm_bool  close_reader; /* If true, closes this source reader automatically when the composite reader is closed. */
} hmCloseableSourceReader;

typedef struct {
    hmAllocator*             allocator;                   /* The allocator which governs this structure's lifetime. */
    hm_nint                  source_reader_count;         /* Specifies the number of source readers in `closeable_source_readers`. */
    hm_nint                  current_source_reader_index; /* Readers are activated in sequence for reading, and here we remember the current one. */
    hmCloseableSourceReader  closeable_source_readers[1]; /* Enumerates the wrapped readers and whether they should be auto-closed.
                                                             Note that the array is embedded inside hmCompositeReaderData to avoid allocation. */
} hmCompositeReaderData;

static hmError hmCompositeReader_read(hmReader* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read)
{
    if (!size) {
        *out_bytes_read = 0;
        return HM_OK; /* do nothing because we were told to read 0 bytes */
    }
    hmCompositeReaderData* data = (hmCompositeReaderData*)reader->data;
    while (HM_TRUE) {
        if (data->current_source_reader_index >= data->source_reader_count) { /* we have read from all the source readers completely */
            *out_bytes_read = 0;
            return HM_OK;
        }
        hmReader* current_source_reader = &data->closeable_source_readers[data->current_source_reader_index].reader;
        HM_TRY(hmReaderRead(current_source_reader, buffer, size, out_bytes_read));
        if (*out_bytes_read > 0) { /* something was read => all OK, finish for now */
            return HM_OK;
        }
        /* The current source reader has been depleted => go to the next one. */
        HM_TRY(hmAddNint(data->current_source_reader_index, 1, &data->current_source_reader_index));
    }
    return HM_OK;
}

static hmError hmCompositeReader_close(hmReader* reader)
{
    hmCompositeReaderData* data = (hmCompositeReaderData*)reader->data;
    hmError err = HM_OK;
    for (hm_nint i = 0; i < data->source_reader_count; i++) {
        if (data->closeable_source_readers[i].close_reader) {
            err = hmMergeErrors(err, hmReaderClose(&data->closeable_source_readers[i].reader));
        }
    }
    hmFree(data->allocator, data);
    return err;
}

hmError hmCreateCompositeReader(
   hmAllocator*    allocator,
   const hmReader* source_readers,
   const hm_bool*  close_source_readers,
   hm_nint         source_reader_count,
   hmReader*       in_reader
)
{
    if (!source_reader_count) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hm_nint data_size = 0;
    /* "source_reader_count - 1" accounts for "closeable_source_readers[1]" already embedded inside hmCompositeReaderData.
       No safe math for "source_reader_count - 1" because it was validated earlier with "!source_reader_count". */
    HM_TRY(hmAddMulNint(sizeof(hmCompositeReaderData), sizeof(hmCloseableSourceReader), source_reader_count - 1, &data_size));
    hmCompositeReaderData* data = (hmCompositeReaderData*)hmAlloc(allocator, data_size);
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    for (hm_nint i = 0; i < source_reader_count; i++) {
        data->closeable_source_readers[i].reader = source_readers[i];
        data->closeable_source_readers[i].close_reader = close_source_readers[i];
    }
    data->allocator = allocator;
    data->source_reader_count = source_reader_count;
    data->current_source_reader_index = 0;
    in_reader->read = &hmCompositeReader_read;
    in_reader->close = &hmCompositeReader_close;
    in_reader->data = data;
    return HM_OK;
}
