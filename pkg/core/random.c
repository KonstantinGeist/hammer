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

#include <core/random.h>
#include <core/environment.h>

/* Based on Numerical Recipes in C (2nd Ed.) and Random.cs from mono/CoreCLR. */

#define MSEED 161803398

static hm_int32 hmRandomSample(hmRandom* random);

hmError hmCreateRandom(hm_int32 seed, hmRandom* in_random)
{
    hm_int32 seed_array[56];
    hm_int32 mj, mk;
    hm_int32 subtraction = (seed == HM_INT32_MIN) ? HM_INT32_MAX : abs(seed);
    mj = MSEED - subtraction;
    seed_array[55] = mj;
    mk = 1;
    for (hm_nint i = 1; i < 55; i++) {
        hm_nint ii = (21 * i) % 55;
        seed_array[ii] = mk;
        mk = mj - mk;
        if (mk < 0) {
            mk += HM_INT32_MAX;
        }
        mj = seed_array[ii];
    }
    for (hm_nint k = 1; k < 5; k++) {
        for (hm_nint i = 1; i < 56; i++) {
            seed_array[i] -= seed_array[1 + (i + 30) % 55];
            if (seed_array[i] < 0) {
                seed_array[i] += HM_INT32_MAX;
            }
        }
    }
    memcpy(&in_random->seed_array[0], &seed_array[0], sizeof(seed_array));
    in_random->inext = 0;
    in_random->inextp = 31;
    return HM_OK;
}

hmError hmRandomDispose(hmRandom* random)
{
    // Nothing to do.
    return HM_OK;
}

hm_float hmRandomGetNextFloat(hmRandom* random)
{
    hm_float ret_value = hmRandomSample(random);
    return ret_value * (1.0 / HM_INT32_MAX);
}

hm_int32 hmRandomGetNextInt(hmRandom* random)
{
    return hmRandomSample(random);
}

hm_int32 hmRandomGetNextIntWithinRange(hmRandom* random, hm_int32 bound1, hm_int32 bound2)
{
    hm_int32 diff = abs(bound2 - bound1); /* FIXME can break if the diff is larger than HM_INT32_MAX */
    hm_int32 min = bound1 < bound2 ? bound1 : bound2;
    if (diff <= 1) {
        return min;
    }
    return (hm_int32)(hmRandomGetNextFloat(random) * diff) + min;
}

static hm_int32 hmRandomSample(hmRandom* random)
{
    if (++random->inext >= 56) {
        random->inext = 1;
    }
    if (++random->inextp >= 56) {
        random->inextp = 1;
    }
    hm_int32 ret_value = random->seed_array[random->inext] - random->seed_array[random->inextp];
    if (ret_value < 0) {
        ret_value += HM_INT32_MAX;
    }
    random->seed_array[random->inext] = ret_value;
    return ret_value;
}
