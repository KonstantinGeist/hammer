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

#ifndef HM_HTTPREQUEST_H
#define HM_HTTPREQUEST_H

#include <core/common.h>
#include <collections/hashmap.h>
#include <io/reader.h>

typedef struct {
    hmReader  reader;       /* Stores the reader to:
                               1) read the body via hmHTTPRequestGetBodyReader(..)
                               2) dispose of it in hmHTTPRequestDispose(..), if enabled via close_reader */
    hmHashMap headers;      /* hmHashMap<hmString, hmArray<hmString>>. Stores the list of parsed HTTP headers. */
    hm_bool   close_reader; /* Copied from the same argument in hmCreateHTTPRequestFromReader(..) (see). */
} hmHTTPRequest;

/* Creates an HTTP request by reading from the given `reader`.
   If `close_reader` is true, the reader is closed inside hmHTTPRequestDispose(..) automatically.
   `hash_salt` is used to prevent DoS attacks against the `headers` dictionary. */
hmError hmCreateHTTPRequestFromReader(
    hmAllocator*   allocator,
    hmReader       reader,
    hm_bool        close_reader,
    hm_uint32      hash_salt,
    hmHTTPRequest* in_request
);
hmError hmHTTPRequestDispose(hmHTTPRequest* request);
/* Returns a reader which allows to read the body of the request. */
hmReader* hmHTTPRequestGetBodyReader(hmHTTPRequest* request);

#endif /* HM_HTTPREQUEST_H */
