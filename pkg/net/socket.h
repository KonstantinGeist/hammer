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

#ifndef HM_SOCKET_H
#define HM_SOCKET_H

#include <core/common.h>
#include <core/string.h>
#include <io/reader.h>

typedef struct {
    hmAllocator* allocator;
    void*        platform_data; /* Platform-specific data is hidden from public headers. */
} hmSocket;

/* A socket allows for two machines to communicate via the network. */
hmError hmCreateSocket(
    hmAllocator* allocator,
    hmString*    host,
    hm_nint      port,
    hmSocket*    in_socket
);
hmError hmSocketSend(hmSocket* socket, const char* buffer, hm_nint size, hm_nint *out_bytes_sent_opt);
hmError hmSocketRead(hmSocket* socket, char* buffer, hm_nint size, hm_nint* out_bytes_read_opt);
hmError hmSocketCreateReader(hmSocket* socket, hmAllocator* reader_allocator_opt, hmReader* in_reader);
hmError hmSocketDispose(hmSocket* socket);
hmError hmSocketDisposeFunc(void* obj);

#endif /* HM_SOCKET_H */
