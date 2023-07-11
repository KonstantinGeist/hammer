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

#include <net/serversocket.h>
#include <core/utils.h>
#include <platform/unix/socket.h>

#include <sys/socket.h> /* for socket(..) & Co. */
#include <netinet/in.h> /* for sockaddr_in & Co. */
#include <arpa/inet.h>  /* for inet_pton(..) & Co. */
#include <unistd.h>     /* for close(..) & Co. */

#define BACKLOG 1000

typedef struct {
    hmAllocator*       allocator;
    int                socket_fd;
    struct sockaddr_in address;
} hmServerSocketPlatformData;

hmError hmCreateServerSocket(
    hmAllocator*    allocator,
    hm_nint         port,
    hmServerSocket* in_socket
)
{
    hmError err = HM_OK;
    hmServerSocketPlatformData* platform_data = (hmServerSocketPlatformData*)hmAlloc(allocator, sizeof(hmServerSocketPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    platform_data->allocator = allocator;
    hm_bool is_socket_initialized = HM_FALSE;
    if ((platform_data->socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {\
        err = HM_ERROR_PLATFORM_DEPENDENT;
        HM_FINALIZE;
    }
    is_socket_initialized = HM_TRUE;
    int opt = 1;
    if (setsockopt(platform_data->socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
        HM_FINALIZE;
    }
    struct sockaddr_in address;
    hmZeroMemory(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    platform_data->address = address;
    if (bind(platform_data->socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
        HM_FINALIZE;
    }
    if (listen(platform_data->socket_fd, BACKLOG) < 0) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
        HM_FINALIZE;
    }
    in_socket->platform_data = platform_data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (is_socket_initialized) {
            int r = shutdown(platform_data->socket_fd, SHUT_RDWR);
            err = hmMergeErrors(err, r ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK);
        }
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmServerSocketAccept(hmServerSocket* socket, hmSocket* out_socket)
{
    hmServerSocketPlatformData* platform_data = (hmServerSocketPlatformData*)socket->platform_data;
    int socket_fd = 0;
    socklen_t address_length = sizeof(platform_data->address);
    if ((socket_fd = accept(platform_data->socket_fd, (struct sockaddr*)&platform_data->address, (socklen_t*)&address_length)) < 0) {
        return HM_ERROR_PLATFORM_DEPENDENT;
    }
    return hmCreateSocketFromDescriptor(platform_data->allocator, socket_fd, out_socket);
}

hmError hmServerSocketDispose(hmServerSocket* socket)
{
    hmServerSocketPlatformData* platform_data = (hmServerSocketPlatformData*)socket->platform_data;
    int r = shutdown(platform_data->socket_fd, SHUT_RDWR);
    hmFree(platform_data->allocator, platform_data);
    return r ? HM_ERROR_PLATFORM_DEPENDENT : HM_OK;
}
