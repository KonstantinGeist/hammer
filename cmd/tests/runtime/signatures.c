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
#include <core/string.h>
#include <runtime/signature.h>

static void assert_is_valid_signature_desc(const char* signature_desc, hm_bool expected_result)
{
    hmString signature;
    hmError err = hmCreateStringViewFromCString(signature_desc, &signature);
    HM_TEST_ASSERT_OK(err);
    hm_bool is_valid = hmIsValidSignatureDesc(&signature);
    HM_TEST_ASSERT(is_valid == expected_result);
}

static void test_validates_signature_descs()
{
    assert_is_valid_signature_desc("", HM_FALSE);
    assert_is_valid_signature_desc("V", HM_TRUE);
    assert_is_valid_signature_desc("F", HM_TRUE);
    assert_is_valid_signature_desc("VIFB", HM_TRUE);
    assert_is_valid_signature_desc("IFV", HM_FALSE);
    assert_is_valid_signature_desc("FZI", HM_FALSE);
    assert_is_valid_signature_desc("FIf", HM_FALSE);
    assert_is_valid_signature_desc("F{core.String}", HM_TRUE);
    assert_is_valid_signature_desc("{core.String}{core.String}", HM_TRUE);
    assert_is_valid_signature_desc("{core.String}F{core.String}I", HM_TRUE);
    assert_is_valid_signature_desc("{core.String}", HM_TRUE);
    assert_is_valid_signature_desc("{core.String", HM_FALSE);
    assert_is_valid_signature_desc("core.String}", HM_FALSE);
    assert_is_valid_signature_desc("}core.String{", HM_FALSE);
}

HM_TEST_SUITE_BEGIN(signatures)
    HM_TEST_RUN_WITHOUT_OOM(test_validates_signature_descs)
HM_TEST_SUITE_END()
