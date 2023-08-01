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

#include <net/http/httprequest.h>
#include <core/math.h>
#include <core/string.h>
#include <collections/array.h>
#include <io/linereader.h>

#define HM_GET_METHOD_LITERAL "GET "
#define HM_GET_METHOD_LITERAL_SIZE 4

#define HM_HTTP_VERSION_LITERAL " HTTP/1.1"
#define HM_HTTP_VERSION_LITERAL_SIZE 9

static hmError hmHTTPRequestParseRequestLineAndHeaderFields(hmHTTPRequest* request);

hmError hmCreateHTTPRequestFromReader(
    hmAllocator*   allocator,
    hmReader       reader,
    hm_bool        close_reader,
    hm_nint        max_header_size,
    hm_nint        max_header_count,
    hm_uint32      hash_salt,
    hmHTTPRequest* in_request
)
{
    hmError err = hmCreateHashMapWithStringKeys(
        allocator,
        &hmArrayDisposeFunc, /* value_dispose_func */
        sizeof(hmArray),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        hash_salt,
        &in_request->headers
    );
    if (err != HM_OK) {
        if (close_reader) {
            err = hmMergeErrors(err, hmReaderClose(&reader));
        }
        return err;
    }
    in_request->allocator = allocator;
    in_request->reader = reader;
    in_request->close_reader = close_reader;
    in_request->method = HM_HTTP_METHOD_GET;
    in_request->max_header_size = max_header_size;
    in_request->max_header_count = max_header_count;
    err = hmCreateEmptyStringView(&in_request->url);
    /* Must be called the last because depends on the fields above. */
    err = hmMergeErrors(err, hmHTTPRequestParseRequestLineAndHeaderFields(in_request));
    if (err != HM_OK) {
        err = hmMergeErrors(err, hmHTTPRequestDispose(in_request));
    }
    return err;
}

hmError hmHTTPRequestDispose(hmHTTPRequest* request)
{
    hmError err = HM_OK;
    if (request->close_reader) {
        err = hmMergeErrors(err, hmReaderClose(&request->reader));
    }
    err = hmMergeErrors(err, hmStringDispose(&request->url));
    return hmMergeErrors(err, hmHashMapDispose(&request->headers));
}

hmReader* hmHTTPRequestGetBodyReader(hmHTTPRequest* request)
{
    /* We use the same reader from hmCreateHTTPRequestFromReader(..), except now its position must be at the beginning
       of the body when the first call to hmHTTPRequestGetBodyReader(..) is made. */
    return &request->reader;
}

static hmError hmParseHTTPRequestLine(hmHTTPRequest* request, hmString* string)
{
    /* HTTP method. */
    if (hmStringStartsWithCStringAndLength(string, HM_GET_METHOD_LITERAL, HM_GET_METHOD_LITERAL_SIZE)) {
        request->method = HM_HTTP_METHOD_GET;
    } else {
        return HM_ERROR_INVALID_DATA;
    }
    /* HTTP version. */
    if (!hmStringEndsWithCStringAndLength(string, HM_HTTP_VERSION_LITERAL, HM_HTTP_VERSION_LITERAL_SIZE)) {
        return HM_ERROR_INVALID_DATA;
    }
    hm_nint url_length = 0;
    HM_TRY(hmSubNint(hmStringGetLengthInBytes(string), HM_GET_METHOD_LITERAL_SIZE, &url_length));
    HM_TRY(hmSubNint(url_length, HM_HTTP_VERSION_LITERAL_SIZE, &url_length));
    return hmCreateSubstring(
        request->allocator,
        string,
        HM_GET_METHOD_LITERAL_SIZE,
        url_length,
        &request->url
    );
}

static hmError hmParseHTTPHeaderField(hmHTTPRequest* request, hmString* string)
{
    return HM_OK;
}

static hmError hmHTTPRequestParseRequestLineOrHeaderField(hmHTTPRequest* request, hmString* string, hm_nint header_count)
{
    hmError err = HM_OK;
    if (hmStringGetLengthInBytes(string) > request->max_header_size || header_count > request->max_header_count) {
        return HM_ERROR_LIMIT_EXCEEDED;
    }
    hm_bool is_request_line = header_count == 0;
    if (is_request_line) {
        return hmMergeErrors(err, hmParseHTTPRequestLine(request, string));
    } else {
        return hmMergeErrors(err, hmParseHTTPHeaderField(request, string));
    }
}

static hmError hmHTTPRequestParseRequestLineAndHeaderFields(hmHTTPRequest* request)
{
    /* Reusing the default max header size limit as the internal buffer size for reading. */
    char buffer[HM_HTTP_REQUEST_DEFAULT_MAX_HEADER_SIZE];
    hmLineReader line_reader;
    HM_TRY(hmCreateLineReader(
        request->allocator,
        request->reader,
        HM_FALSE, /* close_source_reader = HM_FALSE */
        buffer,
        sizeof(buffer),
        &line_reader
    ));
    hmError err = HM_OK;
    hm_nint header_count = 0;
    hmString string;
    while ((err = hmLineReaderReadLine(&line_reader, &string)) == HM_OK) {
        err = hmHTTPRequestParseRequestLineOrHeaderField(request, &string, header_count);
        HM_TRY_OR_FINALIZE(err, hmMergeErrors(err, hmStringDispose(&string))); /* in the HTTP request, derived strings (if any) are retained, not the original one */
        HM_TRY_OR_FINALIZE(err, hmMergeErrors(err, hmAddNint(header_count, 1, &header_count)));
    }
    /* According to the specification of hmLineReaderReadLine(..), error code HM_ERROR_INVALID_STATE tells that there
       are no more lines in the line reader, so we convert it to HM_OK, because it's not actually an error as far as
       this function is concerned. */
    if (err == HM_ERROR_INVALID_STATE) {
        err = HM_OK;
    }
    if (err == HM_OK && header_count == 0) { /* no headers at all */
        err = HM_ERROR_INVALID_DATA;
    }
    /* cppcheck-suppress unknownMacro */
HM_ON_FINALIZE
    return hmMergeErrors(err, hmLineReaderDispose(&line_reader));
}
