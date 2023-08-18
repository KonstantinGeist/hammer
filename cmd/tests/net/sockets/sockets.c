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

#include "../../common.h"
#include <net/sockets/socket.h>
#include <net/sockets/serversocket.h>
#include <core/environment.h>
#include <threading/thread.h>
#include <threading/waitableevent.h>
#include <threading/workerpool.h>

/* These tests rely on some timing, so sporadically they can fail on busy machines. */

#include <string.h> /* for strlen(..) and strcmp(..) */

#define REQUEST_COUNT 10000
#define THREADING_WAIT_TIMEOUT 1000
#define SOCKET_TIMEOUT 1000
#define PORT 8080
#define QUEUE_SIZE 16
#define LOCALHOST "127.0.0.1"
#define PAYLOAD "Hello, World!"
#define PAYLOAD_SIZE 13

typedef struct {
    hmWaitableEvent* waitable_event;
    hmThread*        thread;
} serverSocketContext;

static hmError server_socket_worker_func(void* work_item)
{
    hmSocket* socket = (hmSocket*)work_item;
    char buffer[1024];
    hm_nint bytes_read = 0;
    hmError err = hmSocketRead(socket, buffer, sizeof(buffer), &bytes_read);
    HM_TEST_ASSERT_OK(err);
    buffer[bytes_read] = 0;
    err = hmSocketSend(socket, buffer, bytes_read, HM_NULL); /* Echoes back. */
    HM_TEST_ASSERT_OK(err);
    return HM_OK;
}

static hmError server_socket_thread_func(void* user_data)
{
    serverSocketContext* context = (serverSocketContext*)user_data;
    hmWaitableEvent* waitable_event = context->waitable_event;
    hmThread* thread = context->thread;
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hm_nint worker_count = hmGetProcessorCount();
    if (worker_count < 4) { /* to have some degree of concurrency if the current CPU doesn't have a lot of CPU cores */
        worker_count = 4;
    }
    hmWorkerPool worker_pool;
    err = hmCreateWorkerPool(
        &allocator,
        worker_count,
        &server_socket_worker_func,
        sizeof(hmSocket),
        &hmSocketDisposeFunc,
        HM_FALSE,
        QUEUE_SIZE,
        &worker_pool
    );
    HM_TEST_ASSERT_OK(err);
    hmServerSocket server_socket;
    err = hmCreateServerSocket(
        &allocator,
        PORT,
        HM_SOCKET_MAX_TIMEOUT,
        &server_socket
    );
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventSignal(waitable_event);
    HM_TEST_ASSERT_OK(err);
    do {
        hmSocket socket;
        err = hmServerSocketAccept(&server_socket, HM_NULL, &socket);
        HM_TEST_ASSERT_OK(err);
        err = hmWorkerPoolEnqueueItem(&worker_pool, &socket);
        HM_TEST_ASSERT_OK(err);
    } while (hmThreadGetState(thread) != HM_THREAD_STATE_ABORT_REQUESTED);
    err = hmWorkerPoolStop(&worker_pool, HM_TRUE);
    HM_TEST_ASSERT_OK(err);
    err = hmWorkerPoolWait(&worker_pool, THREADING_WAIT_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    err = hmWorkerPoolDispose(&worker_pool);
    HM_TEST_ASSERT_OK(err);
    err = hmServerSocketDispose(&server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    return HM_OK;
}

static hm_millis socket_throughput_calculate_times(hm_bool client_socket_write_only)
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmWaitableEvent waitable_event;
    err = hmCreateWaitableEvent(&allocator, &waitable_event);
    HM_TEST_ASSERT_OK(err);
    hmThread thread;
    serverSocketContext context;
    context.waitable_event = &waitable_event;
    context.thread = &thread;
    err = hmCreateThread(
        &allocator,
        HM_NULL,
        &server_socket_thread_func,
        &context,
        &thread
    );
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventWait(&waitable_event, HM_WAITABLE_EVENT_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    hmString host;
    err = hmCreateStringViewFromCString(LOCALHOST, &host);
    HM_TEST_ASSERT_OK(err);
    hm_millis start = hmGetTickCount();
    for (hm_nint i = 0; i < REQUEST_COUNT; i++) {
        if (i == REQUEST_COUNT - 1) {
            err = hmThreadAbort(&thread);
            HM_TEST_ASSERT_OK(err);
        }
        hmSocket socket;
        err = hmCreateSocket(
            &allocator,
            &host,
            PORT,
            HM_SOCKET_MAX_TIMEOUT,
            &socket
        );
        HM_TEST_ASSERT_OK(err);
        char message[1024];
        sprintf(message, "message #%d", (int)i);
        err = hmSocketSend(&socket, message, strlen(message), HM_NULL);
        HM_TEST_ASSERT_OK(err);
        if (!client_socket_write_only) {
            char buffer[1024];
            hm_nint bytes_read = 0;
            err = hmSocketRead(&socket, buffer, sizeof(buffer), &bytes_read);
            HM_TEST_ASSERT_OK(err);
            buffer[bytes_read] = 0;
            HM_TEST_ASSERT(strcmp(message, buffer) == 0);
        }
        err = hmSocketDispose(&socket);
        HM_TEST_ASSERT_OK(err);
    }
    hm_millis end = hmGetTickCount();
    err = hmThreadJoin(&thread, THREADING_WAIT_TIMEOUT);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadDispose(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventDispose(&waitable_event);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    return end - start;
}

static void test_can_send_and_read_from_sockets()
{
    hm_millis client_socket_write_only_time = socket_throughput_calculate_times(HM_TRUE);
    hm_millis time = socket_throughput_calculate_times(HM_FALSE);
    printf("        Throughput: %d requests/sec (single-threaded client, without its write time)\n", (int)((hm_float64)REQUEST_COUNT / ((hm_float64)(int)(time - client_socket_write_only_time) / 1000.0)));
}

static void test_socket_reports_error_if_connecting_to_nonexisting_host()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hmString host;
    hmError err = hmCreateStringViewFromCString("notfound.fail", &host);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmCreateSocket(
        &allocator,
        &host,
        PORT,
        HM_SOCKET_MAX_TIMEOUT,
        &socket
    );
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND || err == HM_ERROR_OUT_OF_MEMORY);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static hmError server_sockets_support_accept_timeout_server_thread_func(void* user_data)
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmServerSocket server_socket;
    err = hmCreateServerSocket(&allocator, PORT, SOCKET_TIMEOUT, &server_socket);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmServerSocketAccept(&server_socket, HM_NULL, &socket);
    HM_TEST_ASSERT(err == HM_ERROR_TIMEOUT);
    err = hmServerSocketDispose(&server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    return HM_OK;
}

static void test_server_socket_supports_accept_timeout()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmThread thread;
    err = hmCreateThread(
        &allocator,
        HM_NULL,
        &server_sockets_support_accept_timeout_server_thread_func,
        HM_NULL,
        &thread
    );
    HM_TEST_ASSERT_OK(err);
    hm_millis start = hmGetTickCount();
    err = hmThreadJoin(&thread, HM_THREAD_JOIN_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    hm_millis time = hmGetTickCount() - start;
    HM_TEST_ASSERT(time > SOCKET_TIMEOUT - 100 && time < SOCKET_TIMEOUT + 100); /* with some leeway */
    err = hmThreadDispose(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

static hmError server_sockets_support_read_timeout_server_thread_func(void* user_data)
{
    serverSocketContext* context = (serverSocketContext*)user_data;
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmServerSocket server_socket;
    err = hmCreateServerSocket(&allocator, PORT, SOCKET_TIMEOUT, &server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventSignal(context->waitable_event);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmServerSocketAccept(&server_socket, HM_NULL, &socket);
    HM_TEST_ASSERT_OK(err);
    char buffer[128] = {0};
    hm_nint bytes_read = 0;
    err = hmSocketRead(&socket, buffer, sizeof(buffer), &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == PAYLOAD_SIZE);
    err = hmSocketRead(&socket, buffer, sizeof(buffer), &bytes_read);
    HM_TEST_ASSERT(err == HM_ERROR_TIMEOUT);
    err = hmSocketDispose(&socket);
    HM_TEST_ASSERT_OK(err);
    err = hmServerSocketDispose(&server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    return HM_OK;
}

static void test_server_socket_supports_read_timeout()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmWaitableEvent waitable_event;
    err = hmCreateWaitableEvent(&allocator, &waitable_event);
    HM_TEST_ASSERT_OK(err);
    serverSocketContext context;
    context.waitable_event = &waitable_event;
    context.thread = HM_NULL;
    hmThread thread;
    err = hmCreateThread(
        &allocator,
        HM_NULL,
        &server_sockets_support_read_timeout_server_thread_func,
        &context,
        &thread
    );
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventWait(&waitable_event, HM_WAITABLE_EVENT_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    hmString host;
    err = hmCreateStringViewFromCString(LOCALHOST, &host);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmCreateSocket(&allocator, &host, PORT, HM_SOCKET_MAX_TIMEOUT, &socket);
    HM_TEST_ASSERT_OK(err);
    hm_nint bytes_sent = 0;
    err = hmSocketSend(&socket, PAYLOAD, PAYLOAD_SIZE, &bytes_sent);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_sent == PAYLOAD_SIZE);
    err = hmSleep(SOCKET_TIMEOUT * 2); /* to make sure the server socket times out */
    HM_TEST_ASSERT_OK(err);
    err = hmSocketDispose(&socket);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread, HM_THREAD_JOIN_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadDispose(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventDispose(&waitable_event);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

static hmError client_socket_reacts_to_disconnect_on_send_server_thread_func(void* user_data)
{
    serverSocketContext* context = (serverSocketContext*)user_data;
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmServerSocket server_socket;
    err = hmCreateServerSocket(&allocator, PORT, SOCKET_TIMEOUT, &server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventSignal(context->waitable_event);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmServerSocketAccept(&server_socket, HM_NULL, &socket);
    HM_TEST_ASSERT_OK(err);
    char buffer[128] = {0};
    hm_nint bytes_read = 0;
    err = hmSocketRead(&socket, buffer, sizeof(buffer), &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == PAYLOAD_SIZE);
    err = hmSocketDispose(&socket);
    HM_TEST_ASSERT_OK(err);
    err = hmServerSocketDispose(&server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    return HM_OK;
}

static void test_client_socket_reacts_to_disconnect_on_send()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmWaitableEvent waitable_event;
    err = hmCreateWaitableEvent(&allocator, &waitable_event);
    HM_TEST_ASSERT_OK(err);
    serverSocketContext context;
    context.waitable_event = &waitable_event;
    context.thread = HM_NULL;
    hmThread thread;
    err = hmCreateThread(
        &allocator,
        HM_NULL,
        &client_socket_reacts_to_disconnect_on_send_server_thread_func,
        &context,
        &thread
    );
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventWait(&waitable_event, HM_WAITABLE_EVENT_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    hmString host;
    err = hmCreateStringViewFromCString(LOCALHOST, &host);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmCreateSocket(&allocator, &host, PORT, HM_SOCKET_MAX_TIMEOUT, &socket);
    HM_TEST_ASSERT_OK(err);
    hm_nint bytes_sent = 0;
    err = hmSocketSend(&socket, PAYLOAD, PAYLOAD_SIZE, &bytes_sent);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_sent == PAYLOAD_SIZE);
    err = hmSleep(SOCKET_TIMEOUT); /* waits a little to make sure the connection is closed by the server */
    HM_TEST_ASSERT_OK(err);
    char buff[1024*1024] = {0};
    while ((err = hmSocketSend(&socket, buff, sizeof(buff), &bytes_sent)) == HM_OK);
    HM_TEST_ASSERT(err == HM_ERROR_DISCONNECTED);
    err = hmSocketDispose(&socket);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread, HM_THREAD_JOIN_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadDispose(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventDispose(&waitable_event);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

static hmError server_socket_reacts_to_disconnect_on_read_server_thread_func(void* user_data)
{
    serverSocketContext* context = (serverSocketContext*)user_data;
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmServerSocket server_socket;
    err = hmCreateServerSocket(&allocator, PORT, SOCKET_TIMEOUT, &server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventSignal(context->waitable_event);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmServerSocketAccept(&server_socket, HM_NULL, &socket);
    HM_TEST_ASSERT_OK(err);
    char buffer[128] = {0};
    hm_nint bytes_read = 0;
    err = hmSocketRead(&socket, buffer, sizeof(buffer), &bytes_read);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_read == PAYLOAD_SIZE);
    err = hmSleep(SOCKET_TIMEOUT * 2); /* to make sure the client socket is closed */
    HM_TEST_ASSERT_OK(err);
    err = hmSocketRead(&socket, buffer, sizeof(buffer), &bytes_read);
    HM_TEST_ASSERT(err == HM_OK);
    HM_TEST_ASSERT(bytes_read == 0);
    err = hmSocketDispose(&socket);
    HM_TEST_ASSERT_OK(err);
    err = hmServerSocketDispose(&server_socket);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
    return HM_OK;
}

static void test_server_socket_reacts_to_disconnect_on_read()
{
    hmAllocator allocator;
    hmError err = hmCreateSystemAllocator(&allocator);
    HM_TEST_ASSERT_OK(err);
    hmWaitableEvent waitable_event;
    err = hmCreateWaitableEvent(&allocator, &waitable_event);
    HM_TEST_ASSERT_OK(err);
    serverSocketContext context;
    context.waitable_event = &waitable_event;
    context.thread = HM_NULL;
    hmThread thread;
    err = hmCreateThread(
        &allocator,
        HM_NULL,
        &server_socket_reacts_to_disconnect_on_read_server_thread_func,
        &context,
        &thread
    );
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventWait(&waitable_event, HM_WAITABLE_EVENT_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    hmString host;
    err = hmCreateStringViewFromCString(LOCALHOST, &host);
    HM_TEST_ASSERT_OK(err);
    hmSocket socket;
    err = hmCreateSocket(&allocator, &host, PORT, HM_SOCKET_MAX_TIMEOUT, &socket);
    HM_TEST_ASSERT_OK(err);
    hm_nint bytes_sent = 0;
    err = hmSocketSend(&socket, PAYLOAD, PAYLOAD_SIZE, &bytes_sent);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(bytes_sent == PAYLOAD_SIZE);
    err = hmSocketDispose(&socket);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadJoin(&thread, HM_THREAD_JOIN_MAX_TIMEOUT_MS);
    HM_TEST_ASSERT_OK(err);
    err = hmThreadDispose(&thread);
    HM_TEST_ASSERT_OK(err);
    err = hmWaitableEventDispose(&waitable_event);
    HM_TEST_ASSERT_OK(err);
    err = hmAllocatorDispose(&allocator);
    HM_TEST_ASSERT_OK(err);
}

HM_TEST_SUITE_BEGIN(sockets)
    HM_TEST_RUN_WITHOUT_OOM(test_server_socket_reacts_to_disconnect_on_read)
    HM_TEST_RUN_WITHOUT_OOM(test_client_socket_reacts_to_disconnect_on_send)
    HM_TEST_RUN_WITHOUT_OOM(test_server_socket_supports_read_timeout)
    HM_TEST_RUN_WITHOUT_OOM(test_server_socket_supports_accept_timeout)
    HM_TEST_RUN(test_socket_reports_error_if_connecting_to_nonexisting_host)
    HM_TEST_RUN_WITHOUT_OOM(test_can_send_and_read_from_sockets)
HM_TEST_SUITE_END()
