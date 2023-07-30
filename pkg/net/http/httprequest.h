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
#include <collections/hashmap.h>
#include <io/reader.h>
#include <net/http/common.h>

/* See hmCreateHTTPRequestFromReader(..). The limit is close to that of popular web servers. */
#define HM_HTTP_REQUEST_DEFAULT_MAX_HEADER_SIZE (8*1024)
/* See hmCreateHTTPRequestFromReader(..). The limit is close to that of popular web servers. */
#define HM_HTTP_REQUEST_DEFAULT_MAX_HEADER_COUNT 100

typedef struct {
    hmAllocator* allocator;
    hmReader     reader;           /* Stores the reader in order to:
                                      1) read the body via hmHTTPRequestGetBodyReader(..)
                                      2) dispose of it in hmHTTPRequestDispose(..), if enabled via close_reader */
    hmHashMap    headers;          /* hmHashMap<hmString, hmArray<hmString>>. Stores the list of parsed HTTP headers. */
    hmHTTPMethod method;           /* The HTTP method: GET, POST, PUT etc. */
    hm_nint      max_header_size;  /* The maximum size of a single HTTP header (both key + value). */
    hm_nint      max_header_count; /* The maximum count of HTTP headers. */
    hm_bool      close_reader;     /* Copied from the same argument in hmCreateHTTPRequestFromReader(..) (see). */
} hmHTTPRequest;

/* Creates an HTTP request by reading from the given `reader`.
   If `close_reader` is true, the reader is closed inside hmHTTPRequestDispose(..) automatically, or if this function fails
   (basically, this HTTP request object owns the reader).
  `max_header_size` specifies the maximum size of a single HTTP header (both key + value). Returns HM_ERROR_LIMIT_EXCEEDED
   if it's exceeded. It's recommended to use HM_HTTP_REQUEST_DEFAULT_MAX_HEADER_SIZE.
  `max_header_count` specifies the maximum count of HTTP headers. Returns HM_ERROR_LIMIT_EXCEEDED if it's exceeded.
  It's recommended to use HM_HTTP_REQUEST_DEFAULT_MAX_HEADER_COUNT.
  `hash_salt` is used to prevent DoS attacks against the `headers` dictionary. */
hmError hmCreateHTTPRequestFromReader(
    hmAllocator*   allocator,
    hmReader       reader,
    hm_bool        close_reader,
    hm_nint        max_header_size,
    hm_nint        max_header_count,
    hm_uint32      hash_salt,
    hmHTTPRequest* in_request
);
hmError hmHTTPRequestDispose(hmHTTPRequest* request);
/* Returns a reader which allows to read the body of the request. */
hmReader* hmHTTPRequestGetBodyReader(hmHTTPRequest* request);

#endif /* HM_HTTP_REQUEST_H */
