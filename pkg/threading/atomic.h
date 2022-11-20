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

#ifndef HM_ATOMIC_H
#define HM_ATOMIC_H

/* A set of "Hammerified" atomic intrinsics. */

#include <stdatomic.h>

typedef atomic_size_t hm_atomic_nint;
typedef atomic_bool hm_atomic_bool;

#define hmAtomicStore(object, value) atomic_store_explicit(object, value, memory_order_relaxed)
#define hmAtomicLoad(object) atomic_load_explicit(object, memory_order_relaxed)
/* Atomically increments the value and returns the new value. */
#define hmAtomicIncrement(object) (atomic_fetch_add_explicit(object, 1, memory_order_relaxed) + 1)
/* Atomically decrements the value and returns the new value. */
#define hmAtomicDecrement(object) (atomic_fetch_sub_explicit(object, 1, memory_order_relaxed) - 1)

#endif /* HM_ATOMIC_H */
