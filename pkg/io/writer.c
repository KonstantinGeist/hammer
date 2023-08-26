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

#include <io/writer.h>
#include <core/stringbuilder.h>

hmError hmWriterWrite(hmWriter* writer, const char* buffer, hm_nint size, hm_nint* out_bytes_written)
{
    return writer->write(writer, buffer, size, out_bytes_written);
}

hmError hmWriterClose(hmWriter *writer)
{
    return writer->close(writer);
}

/* ******************* */
/*    StringWriter.    */
/* ******************* */

typedef struct {
    hmAllocator* allocator;         /* The allocator which governs this structure's lifetime. */
    hmStringBuilder string_builder; /* The string builder which accumulates written data (used in hmStringWriterGetString(..)). */
} hmStringWriterData;

static hmError hmStringWriter_write(struct hmWriter_* writer, const char* buffer, hm_nint size, hm_nint* out_bytes_written)
{
    hmStringWriterData* data = (hmStringWriterData*)writer->data;
    hmError err = hmStringBuilderAppendCStringWithLength(&data->string_builder, buffer, size);
    if (err != HM_OK) {
        *out_bytes_written = 0;
        return err;
    }
    *out_bytes_written = size;
    return HM_OK;
}

static hmError hmStringWriter_close(struct hmWriter_* writer)
{
    hmStringWriterData* data = (hmStringWriterData*)writer->data;
    hmError err = hmStringBuilderDispose(&data->string_builder);
    hmFree(data->allocator, data);
    return err;
}

hmError hmCreateStringWriter(hmAllocator* allocator, hmWriter* in_writer)
{
    hmStringWriterData* data = (hmStringWriterData*)hmAlloc(allocator, sizeof(hmStringWriterData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = hmCreateStringBuilder(allocator, &data->string_builder);
    if (err != HM_OK) {
        hmFree(allocator, data);
        return err;
    }
    data->allocator = allocator;
    in_writer->write = &hmStringWriter_write;
    in_writer->close = &hmStringWriter_close;
    in_writer->data = data;
    return HM_OK;
}

hmError hmStringWriterGetString(hmWriter* writer, hmAllocator* allocator_opt, hmString* in_string)
{
    hmStringWriterData* data = (hmStringWriterData*)writer->data;
    return hmStringBuilderToString(&data->string_builder, allocator_opt, in_string);
}
