#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

/** @note check if exists ? check permissions ?*/
/** @note this function will never return is success */
int exec_start(const char* exec_file, char *const argv[], char *const envp[])
{
    pid_t pid;
    int errsv = 0;
    int ret = 0;
    char *path;

    // int access(const char *pathname, int mode);
    if(access(exec_file, X_OK) == -1) {
        errsv = errno;
        goto fail;
    }

    pid = fork();
    errsv = errno;
    switch(pid) {
        case -1 :
            perror("fork");
            goto fail;
        case 0 :
            /** child proccess */
            close(stdin);
            close(stdout);
            close(stderr);

            ret = execvpe(exec_file, argv, envp);
            errsv = errno;
            goto fail_close_fork;
            break;
        default:
            break;
    }

    return pid;

    fail:
    errno = errsv;
    return -1;

    fail_close_fork:
    /** @todo write to syslog ?*/
    errno = errsv;
    return 0;
}

int exec_stop(pid_t pid)
{
    int errsv = 0;

    if(pid == 0) {
        errsv = EINVAL;
        goto fail;
    }

    return kill(pid, SIGTERM);

    fail:
    errno = errsv;
    return -1;
}

int exec_kill(pid_t pid)
{
    int errsv = 0;

    if(pid == 0) {
        errsv = EINVAL;
        goto fail;
    }

    return kill(pid, SIGKILL);

    fail:
    errno = errsv;
    return -1;
}
