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
#include <errno.h>      /* for error codes */
#include <netdb.h>      /* for getaddrinfo(..) & Co. */
#include <stdio.h>      /* for sprintf(..) */

typedef struct {
    hmAllocator* allocator;
    int          socket_fd;
} hmSocketPlatformData;

static hmError hmMapCurrentSocketErrorCodeToHammer(int error_code);

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
    hm_bool is_addrinfo_initialized = HM_FALSE,
            is_socket_initialized = HM_FALSE;
    struct addrinfo hints;
    hmZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo* addrinfo;
    char port_str[32];
    sprintf(port_str, "%d", (int)port);
    int r = getaddrinfo(hmStringGetCString(host), port_str, &hints, &addrinfo);
    if (r) {
        err = r == EAI_NONAME ? HM_ERROR_NOT_FOUND : HM_ERROR_PLATFORM_DEPENDENT;
        HM_FINALIZE;
    }
    is_addrinfo_initialized = HM_TRUE;
    if ((platform_data->socket_fd = socket(addrinfo->ai_family, SOCK_STREAM, 0)) < 0) {
        err = hmMapCurrentSocketErrorCodeToHammer(errno);
        HM_FINALIZE;
    }
    is_socket_initialized = HM_TRUE;
    if (connect(platform_data->socket_fd, addrinfo->ai_addr, (int)addrinfo->ai_addrlen) < 0) {
        err = hmMapCurrentSocketErrorCodeToHammer(errno);
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
    if (is_addrinfo_initialized) {
        freeaddrinfo(addrinfo);
    }
    return err;
}

hmError hmSocketSend(hmSocket* socket, const char* buf, hm_nint sz, hm_nint *out_bytes_sent)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    ssize_t r = send(platform_data->socket_fd, buf, sz, 0);
    if (out_bytes_sent && r >= 0) {
        *out_bytes_sent = (hm_nint)r;
    }
    return r < 0 ? hmMapCurrentSocketErrorCodeToHammer(errno) : HM_OK;
}

hmError hmSocketRead(hmSocket* socket, char* buf, hm_nint sz, hm_nint* out_bytes_read)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    ssize_t r = read(platform_data->socket_fd, buf, sz);
    if (out_bytes_read && r >= 0) {
        *out_bytes_read = (hm_nint)r;
    }
    return r < 0 ? hmMapCurrentSocketErrorCodeToHammer(errno) : HM_OK;
}

hmError hmSocketDispose(hmSocket* socket)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    int r = close(platform_data->socket_fd);
    hmError err = r ? hmMapCurrentSocketErrorCodeToHammer(errno) : HM_OK;
    hmFree(platform_data->allocator, platform_data);
    return err;
}

hmError hmSocketDisposeFunc(void* obj)
{
    return hmSocketDispose((hmSocket*)obj);
}

static hmError hmMapCurrentSocketErrorCodeToHammer(int error_code)
{
    switch (errno) {
        case ETIMEDOUT:
            return HM_ERROR_TIMEOUT;
        case ENETUNREACH:
        case ECONNREFUSED:
            return HM_ERROR_NOT_FOUND;
        default:
            return HM_ERROR_PLATFORM_DEPENDENT;
    }
}
