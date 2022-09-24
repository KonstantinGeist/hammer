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

#ifndef HM_ALLOCATOR_H
#define HM_ALLOCATOR_H

#include "common.h"

/* This header file and the accompanying source file contain several different allocators for different purposes
   which can be, however, interchangeable thanks to the hmAllocator interface. */

/* Generic structure for any allocator. The general idea behind allocators is that objects should not be aware of
   how memory is actually allocated, to allow fast, interchangeable implementations, or implementations specific
   to a certain case. For example, runtime metadata is allocated using a fast bump pointer allocator which can
   be deallocated all at once. */
typedef struct _hmAllocator {
    void* (*alloc)(struct _hmAllocator* allocator, hm_nint sz); /* Allocating function. Returns HM_NULL if out of memory. */
    void  (*free)(struct _hmAllocator* allocator, void* mem);   /* Frees a given block of memory. Behavior is undefined if memory
                                                                   not belonging to this allocator is passed to it. Generally should
                                                                   ignore errors (preferably by logging errors). Safe to pass HM_NULL to it. */
    hmError (*dispose)(struct _hmAllocator* allocator);  /* Function to delete the allocator itself, including all of its
                                                            bookkeeping data. Behavior is undefined if there are still pointers
                                                            to objects allocated through this allocator. */
    void* data;                                          /* Pointer to data specific to the implementation (initialized in
                                                            an allocator's constructor, deleted by dispose(). */
} hmAllocator;

/* Allocates memory given the allocator and the memory size. Returns HM_NULL if out of memory. */
void* hmAlloc(hmAllocator* allocator, hm_nint sz);
/* Reallocates the given memory block: allocates a new array, copies old data to it, and frees the old memory block.
   The memory block can be HM_NULL, in that case it's equivalent to hmAlloc. */
void* hmRealloc(hmAllocator* allocator, void* mem, hm_nint old_size, hm_nint new_size);
/* Frees memory given the allocator and the pointer to the memory block. Behavior is undefined if memory not belonging to
   this allocator is passed to it. Safe to pass NULL to it */
void hmFree(hmAllocator* allocator, void* mem);
/* Deletes the allocator, including all of its bookkeeping data. Behavior is undefined if there are still pointers
   to objects allocated through this allocator. */
hmError hmAllocatorDispose(hmAllocator* allocator);

/* Creates a system allocator - it merely redirects to the OS or the standard library which implement alloc/free.
   Memory alignment is OS-specific. */
hmError hmCreateSystemAllocator(hmAllocator* in_allocator);
/* Creates a simple, but fast bump pointer allocator. Allocations are fast (just a pointer is bumped), and frees
   are no-ops. Useful for static objects which are allocated together and deleted at once (for example, class
   metadata. */
hmError hmCreateBumpPointerAllocator(hmAllocator* base_allocator, hmAllocator* in_allocator);

#endif /* HM_ALLOCATOR_H */
