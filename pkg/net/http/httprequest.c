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
    err = hmCreateEmptyStringView(&in_request->url); /* doesn't need to be disposed on error */
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

hmError hmHTTPRequestGetHeaderRef(hmHTTPRequest* request, hmString* key, hm_nint index, hmString** header_ref)
{
    void* values_ref;
    HM_TRY(hmHashMapGetRef(&request->headers, key, &values_ref));
    if (index >= hmArrayGetCount((hmArray*)values_ref)) {
        return HM_ERROR_NOT_FOUND;
    }
    *header_ref = hmArrayGetRaw((hmArray*)values_ref, hmString);
    return HM_OK;
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

/* Parses header fields in the following formats:
   - "Key: Value"
   - "Key:Value"
   Anything else is not supported. */
static hmError hmParseHTTPHeaderField(hmHTTPRequest* request, hmString* string)
{
    hm_nint colon_index = 0;
    hmError err = hmStringIndexRune(string, (hm_rune)':', &colon_index);
    if (err == HM_ERROR_NOT_FOUND) {
        return HM_ERROR_INVALID_DATA;
    }
    hm_nint whitespace_index = 0;
    HM_TRY(hmAddNint(colon_index, 1, &whitespace_index));
    if (whitespace_index >= hmStringGetLengthInBytes(string) || hmStringGetChars(string)[whitespace_index] != ' ') {
        whitespace_index = colon_index;
    }
    hmString key, value;
    void* values_ref;
    hmArray values;
    hm_bool is_key_owned_by_function = HM_FALSE,
            is_value_owned_by_function = HM_FALSE,
            is_values_owned_by_function = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmCreateSubstring(request->allocator, string, 0, colon_index, &key));
    is_key_owned_by_function = HM_TRUE;
    hm_nint value_start_index = 0, value_length = 0;
    HM_TRY_OR_FINALIZE(err, hmAddNint(whitespace_index, 1, &value_start_index));
    HM_TRY_OR_FINALIZE(err, hmSubNint(hmStringGetLengthInBytes(string), whitespace_index, &value_length));
    HM_TRY_OR_FINALIZE(err, hmSubNint(value_length, 1, &value_length));
    HM_TRY_OR_FINALIZE(err, hmCreateSubstring(request->allocator, string, value_start_index, value_length, &value));
    is_value_owned_by_function = HM_TRUE;
    err = hmHashMapGetRef(&request->headers, &key, &values_ref);
    if (err == HM_ERROR_NOT_FOUND) {
        err = HM_OK;
        HM_TRY_OR_FINALIZE(err, hmCreateArray(
            request->allocator,
            sizeof(hmString),
            HM_ARRAY_DEFAULT_CAPACITY,
            &hmStringDisposeFunc,
            &values
        ));
        is_values_owned_by_function = HM_TRUE;
        HM_TRY_OR_FINALIZE(err, hmHashMapPut(&request->headers, &key, &values));
        is_key_owned_by_function = HM_FALSE;
        is_values_owned_by_function = HM_FALSE;
        HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&request->headers, &key, &values_ref));
    } else if (err != HM_OK) {
        HM_FINALIZE;
    }
    HM_TRY_OR_FINALIZE(err, hmArrayAdd((hmArray*)values_ref, &value));
    is_value_owned_by_function = HM_FALSE;
HM_ON_FINALIZE
    if (is_key_owned_by_function) {
        err = hmMergeErrors(err, hmStringDispose(&key));
    }
    if (is_value_owned_by_function) {
        err = hmMergeErrors(err, hmStringDispose(&value));
    }
    if (is_values_owned_by_function) {
        err = hmMergeErrors(err, hmArrayDispose(&values));
    }
    return err;
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
