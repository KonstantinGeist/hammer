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

#include <net/sockets/socket.h>
#include <core/utils.h>
#include <platform/unix/common.h>

#include <arpa/inet.h>  /* for inet_pton(..) & Co. */
#include <netinet/in.h> /* for sockaddr_in & Co. */
#include <sys/socket.h> /* for socket(..), SO_RCVTIMEO, SO_SNDTIMEO & Co. */
#include <errno.h>      /* for errno */
#include <netdb.h>      /* for getaddrinfo(..) & Co. */
#include <stdio.h>      /* for sprintf(..) */
#include <unistd.h>     /* for close(..) & Co. */

typedef struct {
    hmAllocator* allocator;
    int          socket_file_desc;
} hmSocketPlatformData;

static hmError hmSetSocketTimeout(int file_socket_desk, hm_millis timeout_ms);

hmError hmCreateSocketFromDescriptor(
    hmAllocator* allocator,
    int          socket_file_desc,
    hm_millis    timeout_ms,
    hmSocket*    in_socket
)
{
    HM_TRY(hmSetSocketTimeout(socket_file_desc, timeout_ms));
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)hmAlloc(allocator, sizeof(hmSocketPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    platform_data->socket_file_desc = socket_file_desc;
    in_socket->allocator = allocator;
    in_socket->platform_data = platform_data;
    return HM_OK;
}

hmError hmCreateSocket(
    hmAllocator* allocator,
    hmString*    host,
    hm_nint      port,
    hm_millis    timeout_ms,
    hmSocket*    in_socket
)
{
    if (timeout_ms > HM_SOCKET_MAX_TIMEOUT) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmError err = HM_OK;
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)hmAlloc(allocator, sizeof(hmSocketPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hm_bool is_addrinfo_initialized = HM_FALSE,
            is_socket_initialized = HM_FALSE;
    struct addrinfo hints;
    hmZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo* addrinfo;
    char port_str[32] = {0};
    sprintf(port_str, "%d", (int)port);
    int r = getaddrinfo(hmStringGetCString(host), port_str, &hints, &addrinfo);
    if (r) {
        err = (r == EAI_NONAME || r == EAI_AGAIN) ? HM_ERROR_NOT_FOUND : HM_ERROR_PLATFORM_DEPENDENT; /* getaddrinfo(..) has its own error codes */
        HM_FINALIZE;
    }
    is_addrinfo_initialized = HM_TRUE;
    if ((platform_data->socket_file_desc = socket(addrinfo->ai_family, SOCK_STREAM, 0)) == -1) {
        err = hmUnixErrorToHammer(errno);
        HM_FINALIZE;
    }
    is_socket_initialized = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmSetSocketTimeout(platform_data->socket_file_desc, timeout_ms));
    if (connect(platform_data->socket_file_desc, addrinfo->ai_addr, (int)addrinfo->ai_addrlen) == -1) {
        err = hmUnixErrorToHammer(errno);
        HM_FINALIZE;
    }
    in_socket->allocator = allocator;
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

hmError hmSocketSend(hmSocket* socket, const char* buffer, hm_nint size, hm_nint *out_bytes_sent_opt)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    /* MSG_NOSIGNAL avoids SIGPIPE-related crashes when the connection is abruptly closed. */
    ssize_t bytes_send = send(platform_data->socket_file_desc, buffer, size, MSG_NOSIGNAL);
    if (out_bytes_sent_opt) {
        *out_bytes_sent_opt = bytes_send >= 0 ? (hm_nint)bytes_send : 0;
    }
    return bytes_send == -1 ? hmUnixErrorToHammer(errno) : HM_OK;
}

hmError hmSocketRead(hmSocket* socket, char* buffer, hm_nint size, hm_nint* out_bytes_read_opt)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    ssize_t bytes_read = read(platform_data->socket_file_desc, buffer, size);
    if (out_bytes_read_opt) {
        *out_bytes_read_opt = bytes_read >= 0 ? (hm_nint)bytes_read : 0;
    }
    return bytes_read == -1 ? hmUnixErrorToHammer(errno) : HM_OK;
}

hmError hmSocketDispose(hmSocket* socket)
{
    hmSocketPlatformData* platform_data = (hmSocketPlatformData*)socket->platform_data;
    int unix_err = close(platform_data->socket_file_desc);
    hmError err = unix_err == -1 ? hmUnixErrorToHammer(errno) : HM_OK;
    hmFree(socket->allocator, platform_data);
    return err;
}

hmError hmSocketDisposeFunc(void* obj)
{
    return hmSocketDispose((hmSocket*)obj);
}

static hmError hmSetSocketTimeout(int file_socket_desc, hm_millis timeout_ms)
{
    if (!timeout_ms) {
        return HM_OK;
    }
    struct timeval timeval = hmConvertMillisecondsToTimeVal(timeout_ms);
    if (setsockopt(file_socket_desc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeval, sizeof(timeval)) == -1) {
        return hmUnixErrorToHammer(errno);
    }
    if (setsockopt(file_socket_desc, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeval, sizeof(timeval)) == -1) {
        return hmUnixErrorToHammer(errno);
    }
    return HM_OK;
}
