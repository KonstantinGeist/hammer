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
#include <platform/unix/common.h>

#include <arpa/inet.h>  /* for inet_pton(..) & Co. */
#include <netinet/in.h> /* for sockaddr_in & Co. */
#include <sys/socket.h> /* for socket(..) & Co. */
#include <errno.h>      /* for errno */
#include <netdb.h>      /* for getaddrinfo(..) & Co. */
#include <stdio.h>      /* for sprintf(..) */
#include <unistd.h>     /* for close(..) & Co. */

typedef struct {
    hmAllocator* allocator;
    int          socket_file_desc;
} hmSocketPlatformData;

hmError hmCreateSocketFromDescriptor(
    hmAllocator* allocator,
    int          socket_file_desc,
    hmSocket*    in_socket
)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)hmAlloc(allocator, sizeof(hmSocketPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    platform_data->allocator = allocator;
    platform_data->socket_file_desc = socket_file_desc;
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
        err = r == EAI_NONAME ? HM_ERROR_NOT_FOUND : HM_ERROR_PLATFORM_DEPENDENT; /* getaddrinfo(..) has its own error codes */
        HM_FINALIZE;
    }
    is_addrinfo_initialized = HM_TRUE;
    if ((platform_data->socket_file_desc = socket(addrinfo->ai_family, SOCK_STREAM, 0)) == -1) {
        err = hmUnixErrorToHammer(errno);
        HM_FINALIZE;
    }
    is_socket_initialized = HM_TRUE;
    if (connect(platform_data->socket_file_desc, addrinfo->ai_addr, (int)addrinfo->ai_addrlen) == -1) {
        err = hmUnixErrorToHammer(errno);
        HM_FINALIZE;
    }
    in_socket->platform_data = platform_data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (is_socket_initialized) {
            int unix_err = close(platform_data->socket_file_desc);
            err = hmMergeErrors(err, unix_err == -1 ? hmUnixErrorToHammer(errno) : HM_OK);
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
    ssize_t bytes_send = send(platform_data->socket_file_desc, buf, sz, 0);
    if (out_bytes_sent && bytes_send >= 0) {
        *out_bytes_sent = (hm_nint)bytes_send;
    }
    return bytes_send == -1 ? hmUnixErrorToHammer(errno) : HM_OK;
}

hmError hmSocketRead(hmSocket* socket, char* buf, hm_nint sz, hm_nint* out_bytes_read)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    ssize_t bytes_read = read(platform_data->socket_file_desc, buf, sz);
    if (out_bytes_read && bytes_read >= 0) {
        *out_bytes_read = (hm_nint)bytes_read;
    }
    return bytes_read == -1 ? hmUnixErrorToHammer(errno) : HM_OK;
}

hmError hmSocketDispose(hmSocket* socket)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    int unix_err = close(platform_data->socket_file_desc);
    hmError err = unix_err == -1 ? hmUnixErrorToHammer(errno) : HM_OK;
    hmFree(platform_data->allocator, platform_data);
    return err;
}

hmError hmSocketDisposeFunc(void* obj)
{
    return hmSocketDispose((hmSocket*)obj);
}
