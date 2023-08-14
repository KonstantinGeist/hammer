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

#include <net/sockets/serversocket.h>
#include <core/utils.h>
#include <threading/atomic.h>
#include <platform/unix/common.h>
#include <platform/unix/socket.h>

#include <arpa/inet.h>   /* for inet_pton(..) & Co. */
#include <netinet/in.h>  /* for sockaddr_in & Co. */
#include <bits/socket.h> /* for SOMAXCONN */
#include <sys/socket.h>  /* for socket(..), SO_RCVTIMEO etc. & Co. */
#include <errno.h>       /* for errno */
#include <fcntl.h>       /* for open(..), O_RDONLY */
#include <stdlib.h>      /* for atoi(..) */
#include <unistd.h>      /* for read(..), close(..) */

typedef struct {
    hmAllocator*       allocator;
    hm_millis          timeout_ms;
    int                socket_file_desc;
    struct sockaddr_in address;
} hmServerSocketPlatformData;

static hm_nint hmGetMaxConnectionBacklog();

hmError hmCreateServerSocket(
    hmAllocator*    allocator,
    hm_nint         port,
    hm_millis       timeout_ms,
    hmServerSocket* in_socket
)
{
    if (timeout_ms > HM_SOCKET_MAX_TIMEOUT) {
        return HM_ERROR_INVALID_ARGUMENT;
    }
    hmError err = HM_OK;
    hmServerSocketPlatformData* platform_data = (hmServerSocketPlatformData*)hmAlloc(allocator, sizeof(hmServerSocketPlatformData));
    if (!platform_data) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hm_bool is_socket_initialized = HM_FALSE;
    if ((platform_data->socket_file_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err = hmUnixErrorToHammer(errno);
        HM_FINALIZE;
    }
    is_socket_initialized = HM_TRUE;
    int opt = 1;
    if (setsockopt(platform_data->socket_file_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        err = hmUnixErrorToHammer(errno);
        HM_FINALIZE;
    }
    if (timeout_ms) {
        struct timeval timeval = hmConvertMillisecondsToTimeVal(timeout_ms);
        /* SO_RCVTIMEO affects accept(..) on Linux. */
        if (setsockopt(platform_data->socket_file_desc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeval, sizeof(timeval)) == -1) {
            err = hmUnixErrorToHammer(errno);
            HM_FINALIZE;
        }
    }
    struct sockaddr_in address;
    hmZeroMemory(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    platform_data->address = address;
    platform_data->timeout_ms = timeout_ms;
    if (bind(platform_data->socket_file_desc, (struct sockaddr*)&address, sizeof(address)) == -1) {
        err = hmUnixErrorToHammer(errno);
        HM_FINALIZE;
    }
    hm_nint max_connection_backlog = hmGetMaxConnectionBacklog();
    if (listen(platform_data->socket_file_desc, max_connection_backlog) == -1) {
        err = hmUnixErrorToHammer(errno);
        HM_FINALIZE;
    }
    in_socket->allocator = allocator;
    in_socket->platform_data = platform_data;
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (is_socket_initialized) {
            int unix_err = shutdown(platform_data->socket_file_desc, SHUT_RDWR);
            err = hmMergeErrors(err, unix_err == -1 ? hmUnixErrorToHammer(errno) : HM_OK);
            unix_err = close(platform_data->socket_file_desc);
            err = hmMergeErrors(err, unix_err == -1 ? hmUnixErrorToHammer(errno) : HM_OK);
        }
        hmFree(allocator, platform_data);
    }
    return err;
}

hmError hmServerSocketAccept(hmServerSocket* socket, hmAllocator* socket_allocator_opt, hmSocket* out_socket)
{
    hmServerSocketPlatformData* platform_data = (hmServerSocketPlatformData*)socket->platform_data;
    int socket_file_desc = 0;
    socklen_t address_length = sizeof(platform_data->address);
    if ((socket_file_desc = accept(platform_data->socket_file_desc, (struct sockaddr*)&platform_data->address, (socklen_t*)&address_length)) == -1) {
        return hmUnixErrorToHammer(errno);
    }
    return hmCreateSocketFromDescriptor(
        socket_allocator_opt ? socket_allocator_opt : socket->allocator,
        socket_file_desc,
        platform_data->timeout_ms,
        out_socket
    );
}

hmError hmServerSocketDispose(hmServerSocket* socket)
{
    hmServerSocketPlatformData* platform_data = (hmServerSocketPlatformData*)socket->platform_data;
    int unix_err = shutdown(platform_data->socket_file_desc, SHUT_RDWR);
    hmError err = unix_err == -1 ? hmUnixErrorToHammer(errno) : HM_OK;
    unix_err = close(platform_data->socket_file_desc);
    err = hmMergeErrors(err, unix_err == -1 ? hmUnixErrorToHammer(errno) : HM_OK);
    hmFree(socket->allocator, platform_data);
    return err;
}

static hm_nint hmGetMaxConnectionBacklog()
{
    /* We want to avoid using global variables as much as possible, but this setting is very unlikely to change and it's
       usually system-wide, so it's OK to cache this value once to avoid asking the OS for the backlog size every time a server
       socket is created. */
    static hm_atomic_nint cached_max_connection_backlog = 0;
    hm_nint max_connection_backlog = hmAtomicLoad(&cached_max_connection_backlog);
    if (max_connection_backlog) {
        return max_connection_backlog;
    }
    int file_desc = open("/proc/sys/net/core/somaxconn", O_RDONLY);
    hm_bool is_file_opened = HM_FALSE;
    if (!file_desc) {
        /* If it's not a Linux, or there are other unknown errors - just use C's hardcoded constants here and below. */
        max_connection_backlog = SOMAXCONN;
        HM_FINALIZE;
    }
    is_file_opened = HM_TRUE;
    char buffer[32] = {0};
    ssize_t read_bytes = read(file_desc, buffer, sizeof(buffer));
    if (read_bytes == -1) {
        max_connection_backlog = SOMAXCONN;
        HM_FINALIZE;
    }
    /* The following suppression exists because cppcheck doesn't understand that HM_FINALIZE is a goto. */
    /* cppcheck-suppress negativeIndex */
    buffer[read_bytes] = 0; /* Null terminator. */
    /* The following suppression exists because cppcheck doesn't understand that HM_FINALIZE is a goto. */
    /* cppcheck-suppress redundantAssignment */
    max_connection_backlog = atoi(buffer);
    if (max_connection_backlog == 0) {
        max_connection_backlog = SOMAXCONN;
        HM_FINALIZE;
    }
HM_ON_FINALIZE
    if (is_file_opened) {
        close(file_desc); /* The returned value is ignored because we can't do much about errors here. */
    }
    hmAtomicStore(&cached_max_connection_backlog, max_connection_backlog);
    return max_connection_backlog;
}
