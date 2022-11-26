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

#ifndef HM_RANDOM_H
#define HM_RANDOM_H

#include <core/common.h>

typedef struct {
    hm_int32 seed_array[56];
    hm_int32 inext, inextp;
} hmRandom;

/* Creates a random number generator based on `seed`. The seed can be generated with hmGeneratedSeed() */
hmError hmCreateRandom(hm_int32 seed, hmRandom* in_random);
hmError hmRandomDispose(hmRandom* random);
/* Returns a new random float number in the range [0..1) */
hm_float hmRandomGetNextFloat(hmRandom* random);
/* Returns a new random int number currently in the range [0..HM_INT32_MAX) */
hm_int32 hmRandomGetNextInt(hmRandom* random);
/* Creates a new random int number in the range [bound1, bound2]. */
hm_int32 hmRandomGetNextIntWithinRange(hmRandom* random, hm_int32 bound1, hm_int32 bound2);
/* Creates a seed by drawing from the platform's entropy source to seed random number generators.
   If the platform's entry source is unavailable, returns current time. */
hm_int32 hmGenerateSeed();

#endif /* HM_RANDOM_H */
