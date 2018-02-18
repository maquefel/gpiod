#ifndef __DAEMONIZE_H__
#define __DAEMONIZE_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define DM_NO_MASK        01
#define DM_NO_STD_CLOSE   04

static inline int daemonize(const char* dir, int flags) {
    int ret = 0;
    int errsv = 0;

    ret = fork();
    errsv = errno;

    if(ret < 0) {
        errno = errsv;
        return -1;
    }

    if(ret > 0)
        _exit(EXIT_SUCCESS);

    ret = setsid();
    errsv = errno;

    if(ret < 0)
        return -1;

    ret = fork();
    errsv = errno;

    if(ret < 0) {
        errno = errsv;
        return -1;
    }

    if(ret > 0)
        _exit(EXIT_SUCCESS);

    if(dir != NULL)
        chdir(dir);

    if(!(flags & DM_NO_STD_CLOSE)) {
        close(STDIN_FILENO);
        int fd = open("/dev/null", O_RDWR);

        if(fd != 0)
            return -1;

        dup2(STDIN_FILENO, STDOUT_FILENO);
        dup2(STDIN_FILENO, STDERR_FILENO);
    }

    return 0;
}

static inline int pid_file(const char* pidFile)
{
    return 0;
}

#endif
