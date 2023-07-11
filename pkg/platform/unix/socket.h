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

#ifndef HM_PLATFORM_SOCKET_H
#define HM_PLATFORM_SOCKET_H

#include <net/socket.h>

hmError hmCreateSocketFromDescriptor(
    hmAllocator* allocator,
    int          socket_fd,
    hmSocket*    in_socket
);

#endif /* HM_PLATFORM_SOCKET_H */
