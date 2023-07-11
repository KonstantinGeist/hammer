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

#ifndef HM_SERVER_SOCKET_H
#define HM_SERVER_SOCKET_H

#include <core/common.h>
#include <core/allocator.h>
#include <net/socket.h>

typedef struct {
    void* platform_data; /* Platform-specific data is hidden from public headers. */
} hmServerSocket;

hmError hmCreateServerSocket(
    hmAllocator*    allocator,
    hm_nint         port,
    hmServerSocket* in_socket
);
hmError hmServerSocketAccept(hmServerSocket* socket, hmSocket* out_socket);
hmError hmServerSocketDispose(hmServerSocket* socket);

#endif /* HM_SERVER_SOCKET_H */
