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

#include <net/socket.h>

typedef struct {
    hmAllocator* allocator;
    hmSocket*    socket;
} hmSocketReaderData;

static hmError hmSocketReader_read(hmReader* reader, char* buffer, hm_nint size, hm_nint* out_bytes_read)
{
    hmSocketReaderData* data = (hmSocketReaderData*)reader->data;
    return hmSocketRead(data->socket, buffer, size, out_bytes_read);
}

static hmError hmSocketReader_seek(hmReader* reader, hm_nint offset)
{
    return HM_ERROR_NOT_IMPLEMENTED;
}

static hmError hmSocketReader_close(hmReader* reader)
{
    hmSocketReaderData* data = (hmSocketReaderData*)reader->data;
    hmFree(data->allocator, (char*)reader->data);
    return HM_OK;
}

hmError hmSocketCreateReader(hmSocket* socket, hmAllocator* reader_allocator_opt, hmReader* in_reader)
{
    hmAllocator* allocator = reader_allocator_opt ? reader_allocator_opt : socket->allocator;
    hmSocketReaderData* data = hmAlloc(allocator, sizeof(hmSocketReaderData));
    if (!data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    data->allocator = allocator;
    data->socket = socket;
    in_reader->read = &hmSocketReader_read;
    in_reader->seek = &hmSocketReader_seek;
    in_reader->close = &hmSocketReader_close;
    in_reader->data = data;
    return HM_OK;
}
