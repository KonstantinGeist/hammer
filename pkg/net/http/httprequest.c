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
#include <core/utils.h>
#include <collections/array.h>
#include <io/linereader.h>

/* From RFC9112:
   "Although the request-line grammar rule requires that each of the component elements be separated by a single SP octet,
   recipients MAY instead parse on whitespace-delimited word boundaries and, aside from the CRLF terminator,
   treat any form of whitespace as the SP separator while ignoring preceding or trailing whitespace; such whitespace
   includes one or more of the following octets: SP, HTAB, VT (%x0B), FF (%x0C), or bare CR. However, lenient parsing
   can result in request smuggling security vulnerabilities if there are multiple recipients of the message and each has
   its own unique interpretation of robustness"

   From RFC9110:
   "All general-purpose servers MUST support the methods GET and HEAD. All other methods are OPTIONAL".
   So we implement them and a handlful of other most popular methods.
*/
#define HM_GET_METHOD_LITERAL "GET "
#define HM_GET_METHOD_LITERAL_SIZE 4

#define HM_POST_METHOD_LITERAL "POST "
#define HM_POST_METHOD_LITERAL_SIZE 5

#define HM_PUT_METHOD_LITERAL "PUT "
#define HM_PUT_METHOD_LITERAL_SIZE 4

#define HM_DELETE_METHOD_LITERAL "DELETE "
#define HM_DELETE_METHOD_LITERAL_SIZE 7

#define HM_HEAD_METHOD_LITERAL "HEAD "
#define HM_HEAD_METHOD_LITERAL_SIZE 5

#define HM_HTTP_VERSION_LITERAL " HTTP/1.1"
#define HM_HTTP_VERSION_LITERAL_SIZE 9

static hmError hmHTTPRequestParseRequestLineAndHeaderFields(hmHTTPRequest* request);
#define hmIsHTTPWhitespace(ch) ((ch) == ' ' || (ch) == '\t')

static hm_nint valid_http_header_name_char_table[256] = {
/*  0  1   2    3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30 31 */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63 */
    0,  1,  0,  1,  1,  1,  1,  1,  0,  0,  1,  1,  0,  1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,
/*  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95 */
    0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  1,  1,
/*  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  0,  1,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

hmError hmCreateHTTPRequestFromReader(
    hmAllocator*   allocator,
    hmReader       reader,
    hm_bool        close_reader,
    hm_nint        max_headers_size,
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
    in_request->remaining_buffer = HM_NULL;
    in_request->reader = reader;
    in_request->close_reader = close_reader;
    in_request->method = HM_HTTP_METHOD_GET;
    in_request->max_headers_size = max_headers_size;
    in_request->remaining_buffer_size = 0;
    in_request->is_body_reader_created = HM_FALSE;
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
    err = hmMergeErrors(err, hmHashMapDispose(&request->headers));
    if (request->is_body_reader_created) {
        err = hmMergeErrors(err, hmReaderClose(&request->body_reader));
    }
    if (request->remaining_buffer) {
        hmFree(request->allocator, request->remaining_buffer);
    }
    return err;
}

hmReader* hmHTTPRequestGetBodyReader(hmHTTPRequest* request)
{
    /* We use the same reader from hmCreateHTTPRequestFromReader(..), except now its position must be at the beginning
       of the body when the first call to hmHTTPRequestGetBodyReader(..) is made. */
    return &request->body_reader;
}

hmError hmHTTPRequestGetHeaderRef(hmHTTPRequest* request, hmString* name, hm_nint index, hmString** header_ref)
{
    void* values_ref = HM_NULL;
    HM_TRY(hmHashMapGetRef(&request->headers, name, &values_ref));
    if (index >= hmArrayGetCount((hmArray*)values_ref)) {
        return HM_ERROR_NOT_FOUND;
    }
    hmString* values = hmArrayGetRaw((hmArray*)values_ref, hmString);
    *header_ref = &values[index];
    return HM_OK;
}

static hmError hmParseHTTPMethod(hmString* line, hmHTTPMethod* out_method, hm_nint* method_literal_size)
{
    if (hmStringStartsWithCStringAndLength(line, HM_GET_METHOD_LITERAL, HM_GET_METHOD_LITERAL_SIZE)) {
        *out_method = HM_HTTP_METHOD_GET;
        *method_literal_size = HM_GET_METHOD_LITERAL_SIZE;
        return HM_OK;
    } else if (hmStringStartsWithCStringAndLength(line, HM_POST_METHOD_LITERAL, HM_POST_METHOD_LITERAL_SIZE)) {
        *out_method = HM_HTTP_METHOD_POST;
        *method_literal_size = HM_POST_METHOD_LITERAL_SIZE;
        return HM_OK;
    } else if (hmStringStartsWithCStringAndLength(line, HM_PUT_METHOD_LITERAL, HM_PUT_METHOD_LITERAL_SIZE)) {
        *out_method = HM_HTTP_METHOD_PUT;
        *method_literal_size = HM_PUT_METHOD_LITERAL_SIZE;
        return HM_OK;
    } else if (hmStringStartsWithCStringAndLength(line, HM_DELETE_METHOD_LITERAL, HM_DELETE_METHOD_LITERAL_SIZE)) {
        *out_method = HM_HTTP_METHOD_DELETE;
        *method_literal_size = HM_DELETE_METHOD_LITERAL_SIZE;
        return HM_OK;
    } else if (hmStringStartsWithCStringAndLength(line, HM_HEAD_METHOD_LITERAL, HM_HEAD_METHOD_LITERAL_SIZE)) {
        *out_method = HM_HTTP_METHOD_HEAD;
        *method_literal_size = HM_HEAD_METHOD_LITERAL_SIZE;
        return HM_OK;
    } else {
        return HM_ERROR_INVALID_DATA;
    }
}

static hmError hmHTTPRequestParseRequestLine(hmHTTPRequest* request, hmString* line)
{
    /* HTTP method. */
    hm_nint method_literal_size = 0;
    HM_TRY(hmParseHTTPMethod(line, &request->method, &method_literal_size));
    /* HTTP version. */
    if (!hmStringEndsWithCStringAndLength(line, HM_HTTP_VERSION_LITERAL, HM_HTTP_VERSION_LITERAL_SIZE)) {
        return HM_ERROR_INVALID_DATA;
    }
    hm_nint url_length = 0;
    HM_TRY(hmSubNint(hmStringGetLengthInBytes(line), method_literal_size, &url_length));
    HM_TRY(hmSubNint(url_length, HM_HTTP_VERSION_LITERAL_SIZE, &url_length));
    return hmCreateSubstring(
        request->allocator,
        line,
        method_literal_size,
        url_length,
        &request->url
    );
}

/* Canonicalization: "request-id" => "Request-Id", similar to how Go does it (because HTTP header names are case-insensitive, sadly). */
static hmError hmCanonicalizeHTTPHeaderNameInPlace(hmString* name)
{
    char* chars = HM_NULL;
    HM_TRY(hmStringGetCharsForUpdate(name, &chars));
    hm_nint length_in_bytes = hmStringGetLengthInBytes(name);
    hm_bool should_capitalize = HM_TRUE;
    for (hm_nint i = 0; i < length_in_bytes; i++) {
        char c = chars[i];
        if (should_capitalize) {
            hm_bool is_lower_latin = c >= 'a' && c <= 'z';
            if (is_lower_latin) {
                chars[i] = c - 'a' + 'A';
            }
            should_capitalize = HM_FALSE;
        } else {
            hm_bool is_upper_latin = c >= 'A' && c <= 'Z';
            if (is_upper_latin) {
                chars[i] = c - 'A' + 'a';
            }
        }
        if (c == '-') {
            should_capitalize = HM_TRUE;
        }
    }
    return HM_OK;
}

/* Additionally validates that the header name is standard-conformant. */
static hmError hmHTTPRequestCreateHeaderName(hmHTTPRequest* request, hmString* line, hm_nint colon_index, hmString* in_name)
{
    if (colon_index == 0) {
        return HM_ERROR_INVALID_DATA;
    }
    const hm_utf8char* chars = hmStringGetUTF8Chars(line);
    for (hm_nint i = 0; i < colon_index; i++) {
        if (!valid_http_header_name_char_table[chars[i]]) {
            return HM_ERROR_INVALID_DATA;
        }
    }
    HM_TRY(hmCreateSubstring(request->allocator, line, 0, colon_index, in_name));
    hmError err = hmCanonicalizeHTTPHeaderNameInPlace(in_name);
    if (err != HM_OK) {
        err = hmMergeErrors(err, hmStringDispose(in_name));
    }
    return err;
}

/* This function trims optional whitespace ("OWS") from both sides, according to the HTTP protocol. */
static hmError hmHTTPRequestCreateHeaderValue(hmHTTPRequest* request, hmString* line, hm_nint colon_index, hmString* in_value)
{
    hm_nint value_start_index = 0;
    HM_TRY(hmAddNint(colon_index, 1, &value_start_index)); /* +1 skips the colon itself */
    hm_nint line_length_in_bytes = hmStringGetLengthInBytes(line);
    if (line_length_in_bytes == 0 || value_start_index >= line_length_in_bytes) { /* additional checks just in case */
        return HM_ERROR_INVALID_DATA;
    }
    const char* chars = hmStringGetChars(line);
    for (hm_nint i = value_start_index; i < line_length_in_bytes; i++) {
        if (!hmIsHTTPWhitespace(chars[i])) {
            break;
        }
        HM_TRY(hmAddNint(value_start_index, 1, &value_start_index));
    }
    hm_nint value_end_index = line_length_in_bytes; /* i.e. not inclusive */
    /* No safe math for "line_length_in_bytes - 1" because the bounds were checked in "line_length_in_bytes == 0" above. */
    for (hm_nint i = line_length_in_bytes - 1; i >= value_start_index; i--) {
        if (!hmIsHTTPWhitespace(chars[i])) {
            break;
        }
        HM_TRY(hmSubNint(value_end_index, 1, &value_end_index));
    }
    if (value_start_index >= value_end_index) { /* empty header value? => invalid header */
        return HM_ERROR_INVALID_DATA;
    }
    hm_nint value_length_in_bytes = 0;
    HM_TRY(hmSubNint(value_end_index, value_start_index, &value_length_in_bytes));
    return hmCreateSubstring(request->allocator, line, value_start_index, value_length_in_bytes, in_value);
}

static hmError hmHTTPRequestParseHeaderField(hmHTTPRequest* request, hmString* line)
{
    hm_nint colon_index = 0;
    hmError err = hmStringIndexRune(line, (hm_rune)':', &colon_index);
    if (err == HM_ERROR_NOT_FOUND) {
        return HM_ERROR_INVALID_DATA;
    }
    hmString name, value;
    void* values_ref = HM_NULL;
    hmArray values;
    hm_bool is_name_owned_by_function = HM_FALSE,
            is_value_owned_by_function = HM_FALSE,
            is_values_owned_by_function = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmHTTPRequestCreateHeaderName(request, line, colon_index, &name));
    is_name_owned_by_function = HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmHTTPRequestCreateHeaderValue(request, line, colon_index, &value));
    is_value_owned_by_function = HM_TRUE;
    err = hmHashMapGetRef(&request->headers, &name, &values_ref);
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
        HM_TRY_OR_FINALIZE(err, hmHashMapPut(&request->headers, &name, &values));
        is_name_owned_by_function = HM_FALSE;
        /* cppcheck-suppress redundantAssignment */
        is_values_owned_by_function = HM_FALSE;
        HM_TRY_OR_FINALIZE(err, hmHashMapGetRef(&request->headers, &name, &values_ref));
    } else if (err != HM_OK) {
        HM_FINALIZE;
    }
    HM_TRY_OR_FINALIZE(err, hmArrayAdd((hmArray*)values_ref, &value));
    /* cppcheck-suppress redundantAssignment */
    is_value_owned_by_function = HM_FALSE;
HM_ON_FINALIZE
    if (is_name_owned_by_function) {
        err = hmMergeErrors(err, hmStringDispose(&name));
    }
    if (is_value_owned_by_function) {
        err = hmMergeErrors(err, hmStringDispose(&value));
    }
    if (is_values_owned_by_function) {
        err = hmMergeErrors(err, hmArrayDispose(&values));
    }
    return err;
}

static hmError hmHTTPRequestParseRequestLineOrHeaderField(hmHTTPRequest* request, hmString* line, hm_nint header_count)
{
    /* RFC9112: "A recipient of such a bare CR MUST consider that element to be invalid". */
    if (hmStringIndexRune(line, (hm_rune)'\r', HM_NULL) == HM_OK) {
        return HM_ERROR_INVALID_DATA;
    }
    hm_bool is_request_line = header_count == 0;
    if (is_request_line) {
        return hmHTTPRequestParseRequestLine(request, line);
    } else {
        return hmHTTPRequestParseHeaderField(request, line);
    }
}

static hmError hmHTTPRequestOnNextReader(hm_nint previous_reader_index, void* context_opt)
{
    hmHTTPRequest* request = (hmHTTPRequest*)context_opt;
    /* Immediately disposes of the remaining buffer once the first reader (memory reader) is fully read from,
       to avoid having unused memory hanging around. */
    if (previous_reader_index == 0) {
        hmFree(request->allocator, request->remaining_buffer);
        request->remaining_buffer = HM_NULL;
        request->remaining_buffer_size = 0;
    }
    return HM_OK;
}

static hmError hmHTTPRequestCreateBodyReader(hmHTTPRequest* request, hmLineReader* line_reader)
{
    /* Copies what's left in hmLineReader's fixed-size buffer into hmHTTPRequest's own buffer so that reading  of raw
       body could continue where hmLineReader where left off. */
    char* remaining_buffer = HM_NULL;
    hm_nint remaining_buffer_size = 0;
    HM_TRY(hmLineReaderGetBuffered(line_reader, &remaining_buffer, &remaining_buffer_size));
    if (!remaining_buffer_size) {
        return HM_OK;
    }
    request->remaining_buffer = hmAlloc(request->allocator, remaining_buffer_size);
    if (!request->remaining_buffer) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmCopyMemory(request->remaining_buffer, remaining_buffer, remaining_buffer_size);
    request->remaining_buffer_size = remaining_buffer_size;
    hmError err = HM_OK;
    hmReader memory_reader;
    hm_bool is_memory_reader_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmCreateMemoryReader(
        request->allocator,
        request->remaining_buffer,
        request->remaining_buffer_size,
        &memory_reader
    ));
    is_memory_reader_initialized = HM_TRUE;
    hmReader source_readers[2] = {memory_reader, request->reader};
    /* `memory_reader` is owned by the composite reader from now on.
       `request->reader` is closed separately in hmHTTPRequestDispose(..), depending on `close_reader` from the constructor. */
    hm_bool close_source_readers[2] = {HM_TRUE, HM_FALSE};
    HM_TRY_OR_FINALIZE(err, hmCreateCompositeReader(
        request->allocator,
        source_readers,
        close_source_readers,
        2,
        &hmHTTPRequestOnNextReader,
        request,
        &request->body_reader
    ));
    request->is_body_reader_created = HM_TRUE;
HM_ON_FINALIZE
    if (err != HM_OK) {
        if (is_memory_reader_initialized) {
            err = hmMergeErrors(err, hmReaderClose(&memory_reader));
        }
        hmFree(request->allocator, request->remaining_buffer);
        request->remaining_buffer = HM_NULL;
        request->remaining_buffer_size = 0;
    }
    return err;
}

static hmError hmHTTPRequestParseRequestLineAndHeaderFields(hmHTTPRequest* request)
{
    hmReader limited_reader;
    HM_TRY(hmCreateLimitedReader(
        request->allocator,
        request->reader,
        HM_FALSE, /* close_source_reader = HM_TRUE, because the reader will continue to be used after parsing the headers */
        request->max_headers_size,
        &limited_reader
    ));
    /* Reusing the default max header size limit as the internal buffer size for reading. */
    char buffer[HM_HTTP_REQUEST_DEFAULT_MAX_HEADERS_SIZE];
    hmLineReader line_reader;
    hm_bool is_line_reader_initialized = HM_FALSE;
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmCreateLineReader(
        request->allocator,
        limited_reader,
        HM_FALSE, /* close_source_reader = HM_TRUE, same as above */
        buffer,
        sizeof(buffer),
        HM_TRUE,  /* has_crlf_newlines = HM_TRUE, as per the HTTP protocol */
        &line_reader
    ));
    is_line_reader_initialized = HM_TRUE;
    hm_nint header_count = 0;
    hmString string;
    while ((err = hmLineReaderReadLine(&line_reader, &string)) == HM_OK) {
        if (hmStringIsEmpty(&string)) { /* an empty string is a signal that the header part is over */
            HM_TRY_OR_FINALIZE(err, hmStringDispose(&string));
            HM_TRY_OR_FINALIZE(err, hmHTTPRequestCreateBodyReader(request, &line_reader));
            break;
        }
        err = hmHTTPRequestParseRequestLineOrHeaderField(request, &string, header_count);
        HM_TRY_OR_FINALIZE(err, hmStringDispose(&string)); /* in the HTTP request, derived strings (if any) are retained, not the original one */
        HM_TRY_OR_FINALIZE(err, hmAddNint(header_count, 1, &header_count));
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
    if (is_line_reader_initialized) {
        err = hmMergeErrors(err, hmLineReaderDispose(&line_reader));
    }
    return hmMergeErrors(err, hmReaderClose(&limited_reader));
}
