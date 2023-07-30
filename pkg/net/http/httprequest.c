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
    if (err != HM_OK && close_reader) {
        return hmMergeErrors(err, hmReaderClose(&reader));
    }
    in_request->allocator = allocator;
    in_request->reader = reader;
    in_request->close_reader = close_reader;
    in_request->max_header_size = max_header_size;
    in_request->max_header_count = max_header_count;
    /* Must be called the last because depends on the fields above. */
    err = hmHTTPRequestParseRequestLineAndHeaderFields(in_request);
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
    
    return HM_OK;
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
        err = hmMergeErrors(err, hmStringDispose(&string)); /* in the HTTP request, derived strings (if any) are retained, not the original one */
        err = hmMergeErrors(err, hmAddNint(header_count, 1, &header_count));
        if (err != HM_OK) {
            break;
        }
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
    return hmMergeErrors(err, hmLineReaderDispose(&line_reader));
}
