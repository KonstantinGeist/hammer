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
#include <core/utils.h>

#include <string.h> /* for strlen(..) */

#define HASH_SALT 666

static void test_http_request_with_parameters(
    const char* headers,
    hm_nint     max_headers_size,
    hm_nint     read_buffer_size,
    void      (*func)(hmHTTPRequest* request, void* user_data),
    void*       user_data
)
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
    err = hmCreateHTTPRequestFromReaderAndReadBufferSize(
        &allocator,
        memory_reader,
        HM_TRUE, /* close_reader = HM_TRUE */
        max_headers_size,
        read_buffer_size,
        HASH_SALT,
        &request
    );
    HM_TEST_ASSERT_OK_OR_OOM(err);
    is_request_initialized = HM_TRUE;
    func(&request, user_data);
HM_TEST_ON_FINALIZE
    if (is_request_initialized) {
        err = hmHTTPRequestDispose(&request);
        HM_TEST_ASSERT_OK(err);
    }
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_http_request_with_headers_and_func(const char* headers, void(*func)(hmHTTPRequest* request, void* user_data))
{
    test_http_request_with_parameters(
        headers,
        HM_HTTP_REQUEST_DEFAULT_MAX_HEADERS_SIZE,
        HM_HTTP_REQUEST_MAX_READ_BUFFER_SIZE,
        func,
        HM_NULL /* user_data = HM_NULL */
    );
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
        HM_TRUE, /* close_reader = HM_TRUE */
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

static void test_http_request_can_be_created_from_valid_headers_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_GET);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
    hmString name;
    hmError err = hmCreateStringViewFromCString("Accept-Encoding", &name);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &name, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "gzip, deflate, br"));
    err = hmHTTPRequestGetHeaderRef(request, &name, 1, &value_ref);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
    err = hmCreateStringViewFromCString("Non-Existing-Name", &name);
    HM_TEST_ASSERT_OK(err);
    err = hmHTTPRequestGetHeaderRef(request, &name, 0, &value_ref);
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

static void test_http_request_supports_multiple_values_under_single_name_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_GET);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
    hmString name;
    hmError err = hmCreateStringViewFromCString("Name1", &name);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &name, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "1"));
    err = hmHTTPRequestGetHeaderRef(request, &name, 1, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "2"));
    err = hmHTTPRequestGetHeaderRef(request, &name, 2, &value_ref);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
    err = hmCreateStringViewFromCString("Name2", &name);
    HM_TEST_ASSERT_OK(err);
    err = hmHTTPRequestGetHeaderRef(request, &name, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "3"));
    err = hmHTTPRequestGetHeaderRef(request, &name, 1, &value_ref);
    HM_TEST_ASSERT(err == HM_ERROR_NOT_FOUND);
}

static void test_http_request_supports_multiple_values_under_single_name()
{
    const char* headers = 
        "GET /index HTTP/1.1\r\n"
        "Name1: 1\r\n"
        "Name1: 2\r\n"
        "Name2: 3\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_multiple_values_under_single_name_func);
}

static void test_http_request_rejects_malformed_requests()
{
    test_http_request_with_error("RUN /index HTTP/1.1", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/11.1", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\nName Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\nName:    \t  \t\t   \t \r\n", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\nValue:", HM_ERROR_INVALID_DATA);
}

static void test_http_request_supports_post_requests_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_POST);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/news"));
}

static void test_http_request_supports_post_requests()
{
    test_http_request_with_headers_and_func("POST /news HTTP/1.1", &test_http_request_supports_post_requests_func);
}

static void test_http_request_supports_put_requests_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_PUT);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/message/all"));
}

static void test_http_request_supports_put_requests()
{
    test_http_request_with_headers_and_func("PUT /message/all HTTP/1.1", &test_http_request_supports_put_requests_func);
}

static void test_http_request_supports_lf_newlines_inside_fields_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_GET);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
    hmString name;
    hmError err = hmCreateStringViewFromCString("Name", &name);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &name, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "Value\nWith\nLF"));
}

static void test_http_request_supports_lf_newlines_inside_fields()
{
    const char* headers = 
        "GET /index HTTP/1.1\r\n"
        "Name: Value\nWith\nLF\r\n";
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
        "Name: Value\r\n";
    hmError err = hmCreateMemoryReader(&allocator, headers, strlen(headers), &memory_reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    hmHTTPRequest request;
    hm_bool is_request_initialized = HM_FALSE;
    err = hmCreateHTTPRequestFromReader(
        &allocator,
        memory_reader,
        HM_TRUE, /* close_reader = HM_TRUE */
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

static void test_http_request_supports_optional_whitespace_around_header_fields_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_GET);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
    hmString name;
    hmError err = hmCreateStringViewFromCString("Name", &name);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &name, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "\nValue\nWith\nOWS \t\t \n"));
}

static void test_http_request_supports_optional_whitespace_around_header_fields()
{
    const char* headers = 
        "GET /index HTTP/1.1\r\n"
        "Name:    \t\nValue\nWith\nOWS \t\t \n\t\t  \r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_optional_whitespace_around_header_fields_func);
}

static void test_http_request_supports_header_name_canonicalization_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_GET);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
    hmString name;
    hmError err = hmCreateStringViewFromCString("X-My-Request", &name);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &name, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "Value1"));
    err = hmHTTPRequestGetHeaderRef(request, &name, 1, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "Кириллица"));
}

static void test_http_request_supports_header_name_canonicaliaztion()
{
    const char* headers = 
        "GET /index HTTP/1.1\r\n"
        "X-my-request: Value1\r\n"
        "x-My-rEqueSt: Кириллица\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_header_name_canonicalization_func);
}

static void test_http_request_respects_header_name_restrictions()
{
    test_http_request_with_error("GET /index HTTP/1.1\r\n!:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n\":Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n#:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n$:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n%:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n&:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n':Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n(:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n):Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n*:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n+:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n-:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n.:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n,:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n/:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n0:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n9:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n::Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n;:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n<:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n=:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n>:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n`:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\na:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\nz:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n{:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n|:Value", HM_OK);
    test_http_request_with_error("GET /index HTTP/1.1\r\n}:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\n\xc8:Value", HM_ERROR_INVALID_DATA);
    test_http_request_with_error("GET /index HTTP/1.1\r\nName:Va\rlue", HM_ERROR_INVALID_DATA); /* bare CR */
    test_http_request_with_error("GET /index HTTP/1.1\r\nName1:Value1\r\n Name2:Value2", HM_ERROR_INVALID_DATA); /* line folding */
}

static void test_http_request_supports_post_method_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_POST);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
}

static void test_http_request_supports_post_method()
{
    const char* headers = 
        "POST /index HTTP/1.1\r\n"
        "Key: Value\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_post_method_func);
}

static void test_http_request_supports_put_method_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_PUT);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
}

static void test_http_request_supports_put_method()
{
    const char* headers = 
        "PUT /index HTTP/1.1\r\n"
        "Key: Value\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_put_method_func);
}

static void test_http_request_supports_delete_method_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_DELETE);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
}

static void test_http_request_supports_delete_method()
{
    const char* headers = 
        "DELETE /index HTTP/1.1\r\n"
        "Key: Value\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_delete_method_func);
}

static void test_http_request_supports_head_method_func(hmHTTPRequest* request, void* user_data)
{
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_HEAD);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/index"));
}

static void test_http_request_supports_head_method()
{
    const char* headers = 
        "HEAD /index HTTP/1.1\r\n"
        "Key: Value\r\n";
    test_http_request_with_headers_and_func(headers, &test_http_request_supports_head_method_func);
}

static void test_http_request_rejects_invalid_arguments()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    hmReader memory_reader;
    const char* headers = 
        "POST /send_message HTTP/1.1\r\n"
        "Auth: 12345Q\r\n"
        "\r\n"
        "Hello, World!";
    hmError err = hmCreateMemoryReader(&allocator, headers, strlen(headers), &memory_reader);
    HM_TEST_ASSERT_OK(err);
    hmHTTPRequest request;
    err = hmCreateHTTPRequestFromReaderAndReadBufferSize(
        &allocator,
        memory_reader,
        HM_FALSE, /* close_reader = HM_FALSE */
        4,
        HM_HTTP_REQUEST_DEFAULT_MAX_HEADERS_SIZE + 1,
        HASH_SALT,
        &request
    );
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_ARGUMENT);
    err = hmCreateHTTPRequestFromReaderAndReadBufferSize(
        &allocator,
        memory_reader,
        HM_FALSE, /* close_reader = HM_FALSE */
        0,
        4,
        HASH_SALT,
        &request
    );
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_ARGUMENT);
    err = hmCreateHTTPRequestFromReaderAndReadBufferSize(
        &allocator,
        memory_reader,
        HM_FALSE, /* close_reader = HM_FALSE */
        4,
        0,
        HASH_SALT,
        &request
    );
    HM_TEST_ASSERT(err == HM_ERROR_INVALID_ARGUMENT);
    err = hmReaderClose(&memory_reader);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

static void test_http_request_can_read_body_func(hmHTTPRequest* request, void* user_data)
{
    hm_nint buffer_size = *((hm_nint*)user_data);
    const char* expected_body = "Hello, World!";
    HM_TEST_ASSERT(hmHTTPRequestGetMethod(request) == HM_HTTP_METHOD_POST);
    HM_TEST_ASSERT(hmStringEqualsToCString(hmHTTPRequestGetURL(request), "/send_message"));
    hmReader* body_reader_ref = hmHTTPRequestGetBodyReaderRef(request);
    char buffer[HM_HTTP_REQUEST_MAX_READ_BUFFER_SIZE] = {0};
    hm_nint bytes_read = 0, total_bytes_read = 0;
    hmError err = HM_OK;
    while ((err = hmReaderRead(body_reader_ref, buffer + total_bytes_read, buffer_size, &bytes_read)) == HM_OK && bytes_read > 0) {
        total_bytes_read += bytes_read;
        HM_TEST_ASSERT(total_bytes_read < HM_HTTP_REQUEST_MAX_READ_BUFFER_SIZE);
    }
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(total_bytes_read == strlen(expected_body));
    HM_TEST_ASSERT(hmCompareMemory(buffer, expected_body, total_bytes_read) == 0);
    hmString name;
    err = hmCreateStringViewFromCString("Auth", &name);
    HM_TEST_ASSERT_OK(err);
    hmString* value_ref;
    err = hmHTTPRequestGetHeaderRef(request, &name, 0, &value_ref);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(value_ref, "12345Q"));
}

static void test_http_request_can_read_body()
{
    /* Tests reading with different read buffer sizes to catch edge cases. */
    for (hm_nint i = 2; i < HM_HTTP_REQUEST_MAX_READ_BUFFER_SIZE; i++) {
        const char* headers = 
            "POST /send_message HTTP/1.1\r\n"
            "Auth: 12345Q\r\n"
            "\r\n"
            "Hello, World!";
        test_http_request_with_parameters(
            headers,
            HM_HTTP_REQUEST_DEFAULT_MAX_HEADERS_SIZE,
            i,
            &test_http_request_can_read_body_func,
            &i
        );
    }
}

HM_TEST_SUITE_BEGIN(http_requests)
    HM_TEST_RUN(test_http_request_can_be_created_from_reader)
    HM_TEST_RUN(test_http_request_supports_multiple_values_under_single_name)
    HM_TEST_RUN(test_http_request_rejects_malformed_requests)
    HM_TEST_RUN(test_http_request_supports_post_requests)
    HM_TEST_RUN(test_http_request_supports_put_requests)
    HM_TEST_RUN(test_http_request_supports_lf_newlines_inside_fields)
    HM_TEST_RUN(test_http_request_respects_max_headers_size)
    HM_TEST_RUN(test_http_request_supports_optional_whitespace_around_header_fields)
    HM_TEST_RUN(test_http_request_supports_header_name_canonicaliaztion)
    HM_TEST_RUN_WITHOUT_OOM(test_http_request_respects_header_name_restrictions)
    HM_TEST_RUN_WITHOUT_OOM(test_http_request_supports_post_method)
    HM_TEST_RUN_WITHOUT_OOM(test_http_request_supports_put_method)
    HM_TEST_RUN_WITHOUT_OOM(test_http_request_supports_delete_method)
    HM_TEST_RUN_WITHOUT_OOM(test_http_request_supports_head_method)
    HM_TEST_RUN_WITHOUT_OOM(test_http_request_rejects_invalid_arguments)
    HM_TEST_RUN(test_http_request_can_read_body)
HM_TEST_SUITE_END()
