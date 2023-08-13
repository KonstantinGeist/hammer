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

#define HM_SOCKET_MAX_READ_TIMEOUT (60*60*1000) /* 1 hour must be more than enough */

typedef struct {
    hmAllocator* allocator;
    void*        platform_data; /* Platform-specific data is hidden from public headers. */
} hmSocket;

/* A socket allows for two machines to communicate via the network, with the given `host` and `port`.
   `read_timeout_ms` specifies for how long to wait before hmSocketRead(..) returns HM_ERROR_TIMEOUT (data can be partially read).
   If it's 0, no timeout is set. Can't be greater than HM_SOCKET_MAX_READ_TIMEOUT. */
hmError hmCreateSocket(
    hmAllocator* allocator,
    hmString*    host,
    hm_nint      port,
    hm_millis    read_timeout_ms,
    hmSocket*    in_socket
);
/* Sends the given block specified as buffer[0:size) to the socket.
   `out_bytes_sent_opt` is an optional value which returns how many bytes were actually sent (does it need a retry?)
   The function is synchronous (blocking). */
hmError hmSocketSend(hmSocket* socket, const char* buffer, hm_nint size, hm_nint *out_bytes_sent_opt);
/* Reads the given number of bytes, specified as buffer[0:size) to the socket. The number of read bytes can be 0 --
   that means there's no more data in the socket.
   The function is synchronous (blocking).
   Returns HM_ERROR_TIMEOUT if `read_timeout_ms` of the socket (see hmCreateSocket(..)) is non-zero and it takes more
   time than `read_timeout_ms` milliseconds to read from the socket. */
hmError hmSocketRead(hmSocket* socket, char* buffer, hm_nint size, hm_nint* out_bytes_read_opt);
/* Returns the socket as a reader interface, to be able to read from a socket without knowing it's a socket. */
hmError hmSocketCreateReader(hmSocket* socket, hmAllocator* reader_allocator_opt, hmReader* in_reader);
hmError hmSocketDispose(hmSocket* socket);
hmError hmSocketDisposeFunc(void* obj);

#endif /* HM_SOCKET_H */
