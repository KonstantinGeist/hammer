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

static void test_http_request_with_headers_and_func(const char* headers, void(*func)(hmHTTPRequest* request))
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
        HM_HTTP_REQUEST_DEFAULT_MAX_HEADERS_SIZE,
        HASH_SALT,
        &request
    );
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_request_initialized = HM_TRUE;
    func(&request);
HM_TEST_ON_FINALIZE
    if (is_request_initialized) {
        err = hmHTTPRequestDispose(&request);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_http_request_with_error(const char* headers, hmError expected_error)
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
        HM_HTTP_REQUEST_DEFAULT_MAX_HEADERS_SIZE,
        HASH_SALT,
        &request
    );
    HM_TEST_ASSERT_ERROR_OR_OOM(expected_error, err);
    if (err == HM_OK) {
        is_request_initialized = HM_TRUE;
    }
HM_TEST_ON_FINALIZE
    if (is_request_initialized) {
        err = hmHTTPRequestDispose(&request);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_http_request_can_be_created_from_valid_headers_func(hmHTTPRequest* request)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_GET);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
    hmString key;
    hmError err = hmCreateStringViewFromCString("Accept-Encoding", &key);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &key, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "gzip, deflate, br"));
    err = hmHTTPRequestGetHeaderRef(request, &key, 1, &value_ref);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
    err = hmCreateStringViewFromCString("Non-Existing-Key", &key);
    HM_TEST_ASSERT_OK(err);
    err = hmHTTPRequestGetHeaderRef(request, &key, 0, &value_ref);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
}

static void test_http_request_can_be_created_from_reader()
{
    const char* headers =
        "GET /index HTTP/1.1\r\n"
        "Host: 127.0.0.1:8080\r\n"
        "Connection: keep-alive\r\n"
        "sec-ch-ua: \"Not.A/Brand\";v=\"8\", \"Chromium\";v=\"114\", \"Google Chrome\";v=\"114\"\r\n"
        "sec-ch-ua-mobile: ?0\r\n"
        "sec-ch-ua-platform: \"Linux\"\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
        "Sec-Fetch-Site: none\r\n"
        "Sec-Fetch-Mode: navigate\r\n"
        "Sec-Fetch-User: ?1\r\n"
        "Sec-Fetch-Dest: document\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "Accept-Language: en-US,en;q=0.9\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_can_be_created_from_valid_headers_func);
}

static void test_http_request_supports_multiple_values_under_single_key_func(hmHTTPRequest* request)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_GET);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
    hmString key;
    hmError err = hmCreateStringViewFromCString("Key1", &key);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &key, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "1"));
    err = hmHTTPRequestGetHeaderRef(request, &key, 1, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "2"));
    err = hmHTTPRequestGetHeaderRef(request, &key, 2, &value_ref);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
    err = hmCreateStringViewFromCString("Key2", &key);
    HM_TEST_ASSERT_OK(err);
    err = hmHTTPRequestGetHeaderRef(request, &key, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "3"));
    err = hmHTTPRequestGetHeaderRef(request, &key, 1, &value_ref);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
}

static void test_http_request_supports_multiple_values_under_single_key()
{
    const char* headers = 
        "GET /index HTTP/1.1\r\n"
        "Key1: 1\r\n"
        "Key1: 2\r\n"
        "Key2: 3\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_multiple_values_under_single_key_func);
}

static void test_http_request_rejects_malformed_requests()
{
    test_http_request_with_error("RUN /index HTTP/1.1", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/11.1", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\nKey Value", HM_ERROR_INVALID_DATA);
}

static void test_http_request_supports_post_requests_func(hmHTTPRequest* request)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_POST);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/news"));
}

static void test_http_request_supports_post_requests()
{
    test_http_request_with_headers_and_func("POST /news HTTP/1.1", &test_http_request_supports_post_requests_func);
}

static void test_http_request_supports_put_requests_func(hmHTTPRequest* request)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_PUT);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/message/all"));
}

static void test_http_request_supports_put_requests()
{
    test_http_request_with_headers_and_func("PUT /message/all HTTP/1.1", &test_http_request_supports_put_requests_func);
}

static void test_http_request_supports_lf_newlines_inside_fields_func(hmHTTPRequest* request)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_GET);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
    hmString key;
    hmError err = hmCreateStringViewFromCString("Key", &key);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &key, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "Value\nWith\nLF"));
}

static void test_http_request_supports_lf_newlines_inside_fields()
{
    const char* headers = 
        "GET /index HTTP/1.1\r\n"
        "Key: Value\nWith\nLF\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_lf_newlines_inside_fields_func);
}

static void test_http_request_respects_max_headers_size()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmReader memory_reader;
    const char* headers = 
        "GET /index HTTP/1.1\r\n"
        "Key: Value\r\n";
    hmError err = hmCreateMemoryReader(&allocator, headers, strlen(headers), &memory_reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hmHTTPRequest request;
    hm_bool is_request_initialized = HM_FALSE;
    err = hmCreateHTTPRequestFromReader(
        &allocator,
        memory_reader,
        HM_TRUE,
        strlen(headers) / 2,
        HASH_SALT,
        &request
    );
    HM_TEST_ASSERT_ERROR_OR_OOM(HM_ERROR_LIMIT_EXCEEDED, err);
    if (err == HM_OK) {
        is_request_initialized = HM_TRUE;
    }
HM_TEST_ON_FINALIZE
    if (is_request_initialized) {
        err = hmHTTPRequestDispose(&request);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

HM_TEST_SUITE_BEGIN(http_requests)
    HM_TEST_RUN(test_http_request_can_be_created_from_reader)
    HM_TEST_RUN(test_http_request_supports_multiple_values_under_single_key)
    HM_TEST_RUN(test_http_request_rejects_malformed_requests)
    HM_TEST_RUN(test_http_request_supports_post_requests)
    HM_TEST_RUN(test_http_request_supports_put_requests)
    HM_TEST_RUN(test_http_request_supports_lf_newlines_inside_fields)
    HM_TEST_RUN(test_http_request_respects_max_headers_size)
HM_TEST_SUITE_END()
