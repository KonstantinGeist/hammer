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

#include <threading/process.h>
#include <core/allocator.h>
#include <core/math.h>
#include <core/stringbuilder.h>

#include <errno.h>    /* for errno */
#include <fcntl.h>    /* fcntl(..) */
#include <sys/stat.h> /* for stat(..) */
#include <sys/wait.h> /* waitpid(..) */
#include <unistd.h>   /* for fork() and pipe(..) */

#define HM_UNIX_ARGS_BUFFER_SIZE 512

static hmError hmConvertHammerProcessArgsToUnix(hmAllocator* allocator, hmString* path, hmArray* hammer_args, char*** out_unix_args);
static void hmDisposeUnixProcessArgs(hmAllocator* allocator, char** unix_args);
static hmError hmConvertHammerEnvironmentVarsToUnix(hmAllocator* allocator, hmHashMap* hammer_env_vars, char*** out_unix_env_vars);
static void hmDisposeUnixEnvironmentVars(hmAllocator* allocator, char** unix_env_vars, hm_nint count);
static hmError hmStartUnixProcess(const char* path, char** unix_args, char** unix_env_vars, hm_bool wait_for_exit, hmProcess* in_process);

hmError hmStartProcess(
    hmAllocator*           allocator,
    hmString*              path,
    hmArray*               args,
    hmStartProcessOptions* options_opt,
    hmProcess*             in_process
)
{
    char unix_args_buffer[HM_UNIX_ARGS_BUFFER_SIZE];
    hmAllocator buffer_allocator; /* note: not required to dispose */
    HM_TRY(hmCreateBufferAllocator(unix_args_buffer, sizeof(unix_args_buffer), allocator, &buffer_allocator));
    in_process->exit_code = 0;
    in_process->has_exited = HM_FALSE;
    const char* c_path = hmStringGetCString(path);
    char** unix_args = HM_NULL;
    char** unix_env_vars = HM_NULL;
    hmError err = HM_OK;
    HM_TRY_OR_FINALIZE(err, hmConvertHammerProcessArgsToUnix(&buffer_allocator, path, args, &unix_args));
    if (options_opt && options_opt->environment_vars_opt) {
        HM_TRY_OR_FINALIZE(err, hmConvertHammerEnvironmentVarsToUnix(&buffer_allocator, options_opt->environment_vars_opt, &unix_env_vars));
    }
    hm_bool wait_for_exit = options_opt ? options_opt->wait_for_exit : HM_TRUE;
    HM_TRY_OR_FINALIZE(err, hmStartUnixProcess(c_path, unix_args, unix_env_vars, wait_for_exit, in_process));
HM_ON_FINALIZE
    /* cppcheck-suppress shadowFunction */
    /* cppcheck-suppress unreadVariable */
    hmDisposeUnixProcessArgs(&buffer_allocator, unix_args);
    hmDisposeUnixEnvironmentVars(&buffer_allocator, unix_env_vars, HM_NINT_MAX);
    return err;
}

hmError hmProcessDispose(hmProcess* process)
{
    return HM_OK;
}

/* Converts the arguments to the format specified by the `execv` family of functions (list of C strings terminated with a NULL).
   Note that the function assumes the unix arg array won't outlive the original array since it directly references the char buffers inside it. */
static hmError hmConvertHammerProcessArgsToUnix(hmAllocator* allocator, hmString* path, hmArray* hammer_args, char*** out_unix_args)
{
    hm_nint arg_count = hmArrayGetCount(hammer_args);
    hm_nint alloc_size = 0;
    /* Accounts for the null terminator and the convention that the first argument points to the filename. */
    HM_TRY(hmAddMulNint(sizeof(char*) * 2, arg_count, sizeof(char*), &alloc_size));
    char** unix_args = (char**)hmAlloc(allocator, alloc_size);
    if (!unix_args) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmString* raw_hammer_strings = hmArrayGetRaw(hammer_args, hmString);
    /* "The first argument, by convention, should point to the filename associated with the file being executed". */
    unix_args[0] = (char*)hmStringGetCString(path);
    for (hm_nint i = 0; i < arg_count; i++) {
        hmString* hammer_arg = &raw_hammer_strings[i];
        unix_args[i + 1] = (char*)hmStringGetCString(hammer_arg); /* no overflow-safe math for "i + 1" because prevalidated in alloc_size */
    }
    unix_args[arg_count + 1] = HM_NULL; /* shouldn't overflow because arg_count < alloc_size, which was already validated */
    *out_unix_args = unix_args;
    return HM_OK;
}

static void hmDisposeUnixProcessArgs(hmAllocator* allocator, char** unix_args)
{
    if (!unix_args) {
        return;
    }
    hmFree(allocator, unix_args);
}

typedef struct {
    hmStringBuilder* string_builder;
    char**           unix_env_vars;
    hm_nint          unix_env_vars_index;
} hmEnvironmentMapEnumerateFuncContext;

static hmError hmEnvironmentMapEnumerateFunc(void* key, void* value, void* user_data)
{
    const char* key_c_string = hmStringGetCString((hmString*)key);
    const char* value_c_string = hmStringGetCString((hmString*)value);
    hmEnvironmentMapEnumerateFuncContext* context = (hmEnvironmentMapEnumerateFuncContext*)user_data;
    HM_TRY(hmStringBuilderClear(context->string_builder));
    HM_TRY(hmStringBuilderAppendCStrings(context->string_builder, key_c_string, "=", value_c_string, HM_NULL));
    char* c_string = HM_NULL;
    HM_TRY(hmStringBuilderToCString(context->string_builder, HM_NULL, &c_string));
    context->unix_env_vars[context->unix_env_vars_index] = c_string;
    context->unix_env_vars_index++;
    return HM_OK;
}

static hmError hmConvertHammerEnvironmentVarsToUnix(hmAllocator* allocator, hmHashMap* hammer_env_vars, char*** out_unix_env_vars)
{
    hm_nint env_var_count = hmHashMapGetCount(hammer_env_vars);
    hm_nint alloc_size = 0;
    /* Accounts for the null terminator. */
    HM_TRY(hmAddMulNint(sizeof(char*), env_var_count, sizeof(char*), &alloc_size));
    char** unix_env_vars = (char**)hmAlloc(allocator, alloc_size);
    if (!unix_env_vars) {
        return HM_ERROR_OUT_OF_MEMORY;
    }
    hmError err = HM_OK;
    hmStringBuilder string_builder;
    hm_bool is_string_builder_initialized = HM_FALSE;
    HM_TRY_OR_FINALIZE(err, hmCreateStringBuilder(allocator, &string_builder));
    is_string_builder_initialized = HM_TRUE;
    hmEnvironmentMapEnumerateFuncContext context;
    context.string_builder = &string_builder;
    context.unix_env_vars = unix_env_vars;
    context.unix_env_vars_index = 0;
    HM_TRY_OR_FINALIZE(err, hmHashMapEnumerate(hammer_env_vars, &hmEnvironmentMapEnumerateFunc, &context));
    unix_env_vars[env_var_count] = HM_NULL;
HM_ON_FINALIZE
    if (is_string_builder_initialized) {
        err = hmMergeErrors(err, hmStringBuilderDispose(&string_builder));
    }
    if (err != HM_OK) {
        hmDisposeUnixEnvironmentVars(allocator, unix_env_vars, context.unix_env_vars_index);
    } else {
        *out_unix_env_vars = unix_env_vars;
    }
    return err;
}

static void hmDisposeUnixEnvironmentVars(hmAllocator* allocator, char** unix_env_vars, hm_nint count)
{
    if (!unix_env_vars) {
        return;
    }
    for (hm_nint i = 0; i < count && unix_env_vars[i]; i++) {
        hmFree(allocator, unix_env_vars[i]);
    }
    hmFree(allocator, unix_env_vars);
}

static hm_bool hmFileSystemPathExists(const char* path)
{
    /* TODO move to io::FileSystem */
    struct stat stbuf;
    return stat(path, &stbuf) != -1;
}

static hmError hmStartUnixProcess(const char* path, char** unix_args, char** unix_env_vars, hm_bool wait_for_exit, hmProcess* in_process)
{
    /* Preventively checks if the path exists, because otherwise Valgrind reports memory leaks during tests when a dying subprocess fails to find
       the executable.
       In the future, an overriden filesystem injected in the process' constructor can enforce sandboxing rules, for example. */
    if (!hmFileSystemPathExists(path)) {
        return HM_ERROR_NOT_FOUND;
    }
    /* The self-pipe trick for interprocess communication between the current process and
       the started process (see below). */
    int pipefds[2] = {0};
    if (pipe(pipefds)) {
        return HM_ERROR_PLATFORM_DEPENDENT;
    }
    if (fcntl(pipefds[1], F_SETFD, fcntl(pipefds[1], F_GETFD) | FD_CLOEXEC)) {
        return HM_ERROR_PLATFORM_DEPENDENT;
    }
    /* Forks.
     * WARNING No complex functions should be between fork() and execve(..)
     * because it's not thread-safe when it comes to mutexes (potential deadlocks). */
    pid_t pid = fork();
    if (pid < 0) {
        /* Here and below we don't check errors of close(..) to make sure as much resources are closed as possible
           to avoid resource leaks. */
        close(pipefds[0]);
        close(pipefds[1]);
        return HM_ERROR_PLATFORM_DEPENDENT;
    }
    if (pid == 0) { /* Child process. */
        if (unix_env_vars) {
            execve(path, unix_args, unix_env_vars);
        } else {
            execv(path, unix_args);
        }
        /* If we're here, that means we failed to launch the subprocess (otherwise, the address space would have been
           replaced with a different image in execve(..) above). So, we write a byte to the pipe to signal to the parent
           there was an error. */
        write(pipefds[1], &errno, sizeof(int));
        _exit(1);
    } else { /* Parent process. */
        /* Closes the write end, we don't need it in the parent. */
        close(pipefds[1]);
        /* Tries to read one byte from the pipe. If it's successful, that means the child process failed to launch. */
        int bytes_read = 0;
        int err = 0;
        while ((bytes_read = read(pipefds[0], &err, sizeof(errno))) == -1) {
            if (errno != EAGAIN && errno != EINTR) {
                break;
            }
        }
        close(pipefds[0]);
        if (bytes_read) {
            return HM_ERROR_NOT_FOUND;
        }
        if (wait_for_exit) {
            int status = 0;
            while (waitpid(pid, &status, 0) == -1) {
                if (errno != EINTR) {
                    return HM_ERROR_PLATFORM_DEPENDENT;
                }
            }
            if (WIFEXITED(status)) {
                in_process->exit_code = WEXITSTATUS(status);
                in_process->has_exited = HM_TRUE;
            }
        }
    }
    return HM_OK;
}
