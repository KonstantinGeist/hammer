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

#include "../common.h"
#include <io/writer.h>

#include <string.h> /* for strlen(..) */

static void test_string_writer_writes_and_closes()
{
    hmAllocator allocator;
    HM_TEST_INIT_ALLOC(&allocator);
    HM_TEST_TRACK_OOM(&allocator, HM_FALSE);
    hmWriter writer;
    hmError err = hmCreateStringWriter(&allocator, &writer);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_TRACK_OOM(&allocator, HM_TRUE);
    const char* buffer = "Hello, World!";
    hm_nint buffer_size = strlen(buffer);
    hm_nint bytes_written = 0;
    err = hmWriterWrite(&writer, buffer, buffer_size, &bytes_written);
    if (err == HM_ERROR_OUT_OF_MEMORY) {
        HM_TEST_ASSERT_OK_OR_OOM(err);
    } else {
        HM_TEST_ASSERT_OK(err);
        HM_TEST_ASSERT(bytes_written == buffer_size);
    }
    hmString string;
    err = hmStringWriterGetString(&writer, HM_NULL, &string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
    HM_TEST_ASSERT(hmStringEqualsToCString(&string, buffer));
    err = hmStringDispose(&string);
    HM_TEST_ASSERT_OK_OR_OOM(err);
HM_TEST_ON_FINALIZE
    err = hmWriterClose(&writer);
    HM_TEST_ASSERT_OK(err);
    HM_TEST_DEINIT_ALLOC(&allocator);
}

HM_TEST_SUITE_BEGIN(writers)
    HM_TEST_RUN(test_string_writer_writes_and_closes)
HM_TEST_SUITE_END()
