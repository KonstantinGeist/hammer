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

#include "../common.h"

#include <net/socket.h>
#include <net/serversocket.h>
#include <threading/thread.h>
#include <threading/waitableevent.h>

#include <string.h> /* for strlen(..) and strcmp(..) */

#define THREAD_JOIN_TIMEOUT 1000
#define PORT 8080

static hmError server_socket_thread_func(void* user_data)
{
    hmWaitableEvent* waitable_event = (hmWaitableEvent*)user_data;
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmServerSocket server_socket;
    err = hmCreateServerSocket(&allocator, PORT, &server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventSignal(waitable_event);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmServerSocketAccept(&server_socket, &socket);
    HM_TEST_ASSERT_OK(err);
    char buf[1024];
    hm_nint bytes_read = 0;
    err = hmSocketRead(&socket, buf, sizeof(buf), &bytes_read);
    HM_TEST_ASSERT_OK(err);
    buf[bytes_read] = 0;
    err = hmSocketSend(&socket, buf, bytes_read); /* Echoes back. */
    HM_TEST_ASSERT_OK(err);
    err = hmSocketDispose(&socket);
    HM_TEST_ASSERT_OK(err);
    err = hmServerSocketDispose(&server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    return HM_OK;
}

static void test_can_send_and_read_from_sockets()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmWaitableEvent waitable_event;
    err = hmCreateWaitableEvent(&allocator, &waitable_event);
    HM_TEST_ASSERT_OK(err);
    hmThread thread;
    err = hmCreateThread(
        &allocator,
        HM_NULL,
        server_socket_thread_func,
        &waitable_event,
        &thread
    );
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventWait(&waitable_event, HM_WAITABLE_EVENT_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    hmString host;
    err = hmCreateStringViewFromCString("127.0.0.1", &host);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmCreateSocket(
        &allocator,
        &host,
        PORT,
        &socket
    );
    HM_TEST_ASSERT_OK(err);
    const char* message = "Hello, World!";
    err = hmSocketSend(&socket, message, strlen(message));
    HM_TEST_ASSERT_OK(err);
    char buf[1024];
    hm_nint bytes_read = 0;
    err = hmSocketRead(&socket, buf, sizeof(buf), &bytes_read);
    HM_TEST_ASSERT_OK(err);
    buf[bytes_read] = 0;
    HM_TEST_ASSERT(strcmp(message, buf) == 0);
    err = hmSocketDispose(&socket);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread, THREAD_JOIN_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadDispose(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventDispose(&waitable_event);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

HM_TEST_SUITE_BEGIN(sockets)
    HM_TEST_RUN_WITHOUT_OOM(test_can_send_and_read_from_sockets)
HM_TEST_SUITE_END()
