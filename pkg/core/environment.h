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

#ifndef HM_ENVIRONMENT_H
#define HM_ENVIRONMENT_H

#include <core/common.h>
#include <core/string.h>
#include <collections/array.h>

struct _hmAllocator;

/* Gets the number of milliseconds elapsed since a platform-dependent epoch. */
hm_millis hmGetTickCount();
/* Returns the number of processors available in the current environment.
   May return 1 if it's not possible to detect the number of processors. */
hm_nint hmGetProcessorCount();
/* Returns a list of the program's command line arguments as passed to the executable (not including the
   program name).
   `in_array` is an array of strings, which should be disposed by the caller with hmArrayDispose(..) */
hmError hmGetCommandLineArguments(struct _hmAllocator* allocator, hmArray* in_array);
/* Returns an environment variable. More often than not, variable names are fixed constants, so we use
   a C string here for the key instead of a Hammer string, for convenience.
   Returns an empty string if no variable is found. */
hmError hmGetEnvironmentVariable(struct _hmAllocator* allocator, const char* name, hmString* in_value);
/* Gets the file path of the currently running executable. The returned string can be used to spawn another instance
   of the running program. The returned file path points to the final executable loaded by the OS. In the case of
   scripts, the path points to the script handler, not to the script. The returned value points to the actual exectuable
   and not a symlink. */
hmError hmGetExecutableFilePath(struct _hmAllocator* allocator, hmString* in_file_path);

#endif /* HM_ENVIRONMENT_H */
