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
#include <net/sockets/socket.h>

typedef struct {
    hmAllocator* allocator;
    void*        platform_data; /* Platform-specific data is hidden from public headers. */
} hmServerSocket;

/* A server socket is a special socket which is able to accept new connections on the given `port` and establish communication by
   creating client sockets on every connection (via hmServerSocketAccept(..)). When created, a server socket is
   already automatically in the listening state, ready to accept new connections.
  `timeout_ms` specifies the read and write timeouts in milliseconds for hmServerSocketAccept(..) and all created sockets
   (see hmCreateSocket(..) for details). */
hmError hmCreateServerSocket(
    hmAllocator*    allocator,
    hm_nint         port,
    hm_millis       timeout_ms,
    hmServerSocket* in_socket
);
/* Blocks the current thread until a new connection is available. If it's available, returns a new socket object
   which can be used on another thread. */
hmError hmServerSocketAccept(hmServerSocket* socket, hmAllocator* socket_allocator_opt, hmSocket* out_socket);
hmError hmServerSocketDispose(hmServerSocket* socket);

#endif /* HM_SERVER_SOCKET_H */
