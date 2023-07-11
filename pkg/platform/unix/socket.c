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
#include <core/utils.h>

#include <sys/socket.h> /* for socket(..) & Co. */
#include <netinet/in.h> /* for sockaddr_in & Co. */
#include <arpa/inet.h>  /* for inet_pton(..) & Co. */
#include <unistd.h>     /* for close(..) & Co. */

typedef struct {
    hmAllocator* allocator;
    int          socket_fd;
} hmSocketPlatformData;

hmError hmCreateSocketFromDescriptor(
    hmAllocator* allocator,
    int          socket_fd,
    hmSocket*    in_socket
)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)hmAlloc(allocator, sizeof(hmSocketPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    platform_data->allocator = allocator;
    platform_data->socket_fd = socket_fd;
    in_socket->platform_data = platform_data;
    return HM_OK;
}

hmError hmCreateSocket(
    hmAllocator* allocator,
    hmString*    host,
    hm_nint      port,
    hmSocket*    in_socket
)
{
    hmError err = HM_OK;
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)hmAlloc(allocator, sizeof(hmSocketPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    platform_data->allocator = allocator;
    hm_bool is_socket_initialized = HM_FALSE;
    if ((platform_data->socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
        HM_FINALIZE;
    }
    is_socket_initialized = HM_TRUE;
    struct sockaddr_in server_address;
    hmZeroMemory(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, hmStringGetCString(host), &server_address.sin_addr) <= 0) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
        HM_FINALIZE;
    }
    if (connect(platform_data->socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        err = HM_ERROR_PLATFORM_DEPENDENT;
        HM_FINALIZE;
    }
    in_socket->platform_data = platform_data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (is_socket_initialized) {
            close(platform_data->socket_fd);
        }
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmSocketSend(hmSocket* socket, const char* buf, hm_nint sz)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    send(platform_data->socket_fd, buf, sz, 0);
    return HM_OK;
}

hmError hmSocketRead(hmSocket* socket, char* buf, hm_nint sz, hm_nint* out_bytes_read)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    *out_bytes_read = read(platform_data->socket_fd, buf, sz);
    /* TODO error condition */
    return HM_OK;
}

hmError hmSocketDispose(hmSocket* socket)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    close(platform_data->socket_fd);
    hmFree(platform_data->allocator, platform_data);
    return HM_OK;
}
