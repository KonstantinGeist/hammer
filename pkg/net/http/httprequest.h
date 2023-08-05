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

#ifndef HM_HTTP_REQUEST_H
#define HM_HTTP_REQUEST_H

#include <core/common.h>
#include <core/string.h>
#include <collections/hashmap.h>
#include <io/reader.h>
#include <net/http/common.h>

/* See hmCreateHTTPRequestFromReader(..). */
#define HM_HTTP_REQUEST_DEFAULT_MAX_HEADERS_SIZE (8*1024)

typedef struct {
    hmAllocator* allocator;
    hmReader     reader;           /* Stores the reader in order to:
                                      1) read the body via hmHTTPRequestGetBodyReader(..)
                                      2) dispose of it in hmHTTPRequestDispose(..), if enabled via close_reader */
    hmHashMap    headers;          /* hmHashMap<hmString, hmArray<hmString>>. Stores the list of parsed HTTP headers. */
    hmString     url;              /* URL of the request. */
    hmHTTPMethod method;           /* The HTTP method: GET, POST, PUT etc. */
    hm_nint      max_headers_size; /* The maximum size of all HTTP headers. */
    hm_bool      close_reader;     /* Copied from the same argument in hmCreateHTTPRequestFromReader(..) (see). */
} hmHTTPRequest;

/* Creates an HTTP request by reading from the given `reader`.
   HTTP requests in Hammer don't strictly follow the HTTP standard:
   1) optional whitespaces: only 1 whitespace between ":" and header values allowed;
   2) supports UTF8 in header fields;
   3) supports only GET, POST and PUT methods.
   HTTP requests support only basic interoperability with other systems and browsers.
   If `close_reader` is true, the reader is closed inside hmHTTPRequestDispose(..) automatically, or if this function fails
   (basically, this HTTP request object owns the reader).
  `max_headers_size` specifies the maximum size of all HTTP headers in the request (both key + value). Returns HM_ERROR_LIMIT_EXCEEDED
   if it's exceeded. It's recommended to use HM_HTTP_REQUEST_DEFAULT_MAX_HEADERS_SIZE.
  `hash_salt` is used to prevent DoS attacks against the `headers` dictionary. */
hmError hmCreateHTTPRequestFromReader(
    hmAllocator*   allocator,
    hmReader       reader,
    hm_bool        close_reader,
    hm_nint        max_headers_size,
    hm_uint32      hash_salt,
    hmHTTPRequest* in_request
);
hmError hmHTTPRequestDispose(hmHTTPRequest* request);
/* Returns a reader which allows to read the body of the request. */
hmReader* hmHTTPRequestGetBodyReader(hmHTTPRequest* request);
/* Returns a header by its key and index (there can be several values per key) in `header_ref`.
   The value is owned by the HTTP request object and should not be disposed. The value is valid as long as the HTTP request
   object is valid.
   Returns HM_ERROR_NOT_FOUND if no value is found for the given key/index pair.
   Usually, for most headers, zero can be passed for `index`. */
hmError hmHTTPRequestGetHeaderRef(hmHTTPRequest* request, hmString* key, hm_nint index, hmString** header_ref);
#define hmHTTPRequestGetMethod(request) ((request)->method)
#define hmHTTPRequestGetURL(request) (&(request)->url)

#endif /* HM_HTTP_REQUEST_H */
