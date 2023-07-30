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

#include <net/http/httprequest.h>

#include <string.h> /* for strlen(..) */

#define HASH_SALT 666

// TODO add CRLF support as by the spec
static const char* headers =
    "GET / HTTP/1.1\n"
    "Host: 127.0.0.1:8080\n"
    "Connection: keep-alive\n"
    "sec-ch-ua: \"Not.A/Brand\";v=\"8\", \"Chromium\";v=\"114\", \"Google Chrome\";v=\"114\"\n"
    "sec-ch-ua-mobile: ?0\n"
    "sec-ch-ua-platform: \"Linux\"\n"
    "Upgrade-Insecure-Requests: 1\n"
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\n"
    "Sec-Fetch-Site: none\n"
    "Sec-Fetch-Mode: navigate\n"
    "Sec-Fetch-User: ?1\n"
    "Sec-Fetch-Dest: document\n"
    "Accept-Encoding: gzip, deflate, br\n"
    "Accept-Language: en-US,en;q=0.9\n";

static void test_http_request_can_be_created_from_reader()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmReader memory_reader;
    hmError err = hmCreateMemoryReader(&allocator, headers, strlen(headers), &memory_reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hmHTTPRequest request;
    hm_bool is_request_initialized = HM_FALSE;
    err = hmCreateHTTPRequestFromReader(
        &allocator,
        memory_reader,
        HM_TRUE,
        HM_HTTP_REQUEST_DEFAULT_MAX_HEADER_SIZE,
        HM_HTTP_REQUEST_DEFAULT_MAX_HEADER_COUNT,
        HASH_SALT,
        &request
    );
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_request_initialized = HM_TRUE;
HM_TEST_ON_FINALIZE
    if (is_request_initialized) {
        err = hmHTTPRequestDispose(&request);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

HM_TEST_SUITE_BEGIN(http_requests)
    HM_TEST_RUN(test_http_request_can_be_created_from_reader)
HM_TEST_SUITE_END()
