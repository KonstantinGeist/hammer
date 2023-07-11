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

typedef struct {
    void* platform_data; /* Platform-specific data is hidden from public headers. */
} hmSocket;

hmError hmCreateSocket(
    hmAllocator* allocator,
    hmString*    host,
    hm_nint      port,
    hmSocket*    in_socket
);
hmError hmSocketSend(hmSocket* socket, const char* buf, hm_nint sz);
hmError hmSocketRead(hmSocket* socket, char* buf, hm_nint sz, hm_nint* out_bytes_read);
hmError hmSocketDispose(hmSocket* socket);

#endif /* HM_SOCKET_H */
