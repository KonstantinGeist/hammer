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

/* A server socket is a special socket which is able to accept new connections and establish communication by
   creating client sockets on every connection (via hmServerSocketAccept(..)). When created, a server socket is
   already automatically in the listening state, ready to accept new connections. */
hmError hmCreateServerSocket(
    hmAllocator*    allocator,
    hm_nint         port,
    hmServerSocket* in_socket
);
hmError hmServerSocketAccept(hmServerSocket* socket, hmSocket* out_socket);
hmError hmServerSocketDispose(hmServerSocket* socket);

#endif /* HM_SERVER_SOCKET_H */
