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

#include "common.h"

hmError hmCombineErrors(hmError older, hmError newer)
{
    if (newer == HM_OK) { // normal case: `newer` is actually an OK status so we show the `older` error instead
        return older;
    }
    if (older == HM_OK) { // if the `older` error is an OK status, we show the `newer` error -- maybe there's something there
        return newer;
    }
    // The interesting case is when both `older` and `newer` contain an error. What do we show?
    // Usually we want to see the original error because all the subsequent errors may be just a consequence of that original
    // error. However, we don't want to ignore new errors altogether either. In the future, we may start logging such cases
    // in some way.
    return older;
}
