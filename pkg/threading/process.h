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

#ifndef HM_PROCESS_H
#define HM_PROCESS_H

#include <core/common.h>
#include <core/string.h>
#include <collections/hashmap.h>

struct _hmAllocator;

/* Specifies additional options when starting an external process. */
typedef struct {
    hmHashMap* environment_vars; /* hmHashMap<hmString, hmString>. Can be HM_NULL if environment variables don't need to be overriden. */
    hm_bool    wait_for_exit;    /* If HM_TRUE, blocks the current thread until the process finishes. By default, it's HM_TRUE. */
} hmStartProcessOptions;

/* Represents an external process in the system. */
typedef struct {
    /* TODO */
    int dummy;
} hmProcess;

/* Starts a new process to use external tools installed in the system.
   The allocator must be thread-safe.
   `path` is the path to the executable; the search algorithm depends on the OS.
   `args` is the list of string arguments (hmArray<hmString>).
   `options` represents additional options (can be HM_NULL); see hmStartProcessOptions. */
hmError hmStartProcess(
    struct _hmAllocator* allocator,
    hmString* path,
    hmArray* args,
    hmStartProcessOptions* options,
    hmProcess* in_process
);
/* Disposes of the resources held by the representation of the external process inside the current process
   (i.e. doesn't kill the process). */
hmError hmProcessDispose(hmProcess* process);

#endif /* HM_PROCESS_H */
