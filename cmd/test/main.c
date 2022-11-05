// *****************************************************************************
//
//  Copyright (c) Konstantin Geist. All rights reserved.
//
//  The use and distribution terms for this software are contained in the file
//  named License.txt, which can be found in the root of this distribution.
//  By using this software in any fashion, you are agreeing to be bound by the
//  terms of this license.
//
//  You must not remove this notice, or any other, from this software.
//
// *****************************************************************************

#include "tests.h"

int main()
{
    test_allocators();
    test_readers();
    test_arrays();
    test_modules();
    test_strings();
    test_utils();
    test_hashmaps();
    test_hashes();
    test_errors();
    return 0;
}
