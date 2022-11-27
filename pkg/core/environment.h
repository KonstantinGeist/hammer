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

#ifndef HM_ENVIRONMENT_H
#define HM_ENVIRONMENT_H

#include <core/common.h>

/* Gets the number of milliseconds elapsed since a platform-dependent epoch. */
hm_millis hmGetTickCount();
/* Returns the number of processors available in the current environment.
   May return 1 if it's not possible to detect the number of processors. */
hm_nint hmGetProcessorCount();

#endif /* HM_ENVIRONMENT_H */
