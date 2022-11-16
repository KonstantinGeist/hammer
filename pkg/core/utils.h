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

#ifndef HM_UTILS_H
#define HM_UTILS_H

#include <core/common.h>

hm_nint hmAlignSize(hm_nint sz);

/* Useful for logging when there's no other way to report an error. */
void hmLog(const char* msg);

#endif /* HM_UTILS_H */
