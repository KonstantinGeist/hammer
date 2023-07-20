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
#include <collections/array.h>

static hmError hmParseHTTPHeaders(hmAllocator* allocator, hmReader* reader, hmHashMap* in_headers, hmHTTPMethod* out_method);

hmError hmCreateHTTPRequestFromReader(
    hmAllocator*   allocator,
    hmReader       reader,
    hm_bool        close_reader,
    hm_uint32      hash_salt,
    hmHTTPRequest* in_request
)
{
    HM_TRY(hmCreateHashMapWithStringKeys(
        allocator,
        &hmArrayDisposeFunc, /* value_dispose_func */
        sizeof(hmArray),
        HM_HASHMAP_DEFAULT_CAPACITY,
        HM_HASHMAP_DEFAULT_LOAD_FACTOR,
        hash_salt,
        &in_request->headers
    ));
    in_request->reader = reader;
    in_request->close_reader = close_reader;
    hmError err = hmParseHTTPHeaders(allocator, &in_request->reader, &in_request->headers, &in_request->method);
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

static hmError hmParseHTTPHeaders(hmAllocator* allocator, hmReader* reader, hmHashMap* in_headers, hmHTTPMethod* out_method)
{
    return HM_OK;
}
