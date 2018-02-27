#ifndef __DAEMONIZE_H__
#define __DAEMONIZE_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
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

static inline pid_t read_pid_file(const char* pidFile)
{
    int ret = 0;
    int errsv = 0;
    pid_t pid = -1;
    int pid_fd = 0;

    char buffer[16] = {0};

    pid_fd = open(pidFile, O_RDONLY);
    errsv = errno;

    if(pid_fd > 0) {
        ret = read(pid_fd, buffer, sizeof(buffer) - 1);
        errsv = errno;

        if(ret > 0)
            pid = strtol(buffer, NULL, 10);
        errsv = errno;

        if(pid == 0 || errsv == ERANGE)
            pid = -1;

        close(pid_fd);
    }

    errno = errsv;
    return pid;
}

static inline pid_t create_pid_file(const char* pidFile)
{
    int ret = 0;
    int pid_fd = 0;
    int errsv = 0;
    char buffer[16] = {0};
    pid_t pid = 0;

    pid_fd = open(pidFile, O_CREAT | O_WRONLY | O_EXCL, 0644);
    errsv = errno;

    if(pid_fd == -1)
        goto fail;

    pid = getpid();
    ret = sprintf(buffer, "%d\n", pid);
    ret = write(pid_fd, buffer, ret);
    errsv = errno;

    close(pid_fd);

    if(ret == -1)
        goto fail;

    return pid;

    fail:
    errno = errsv;
    return -1;
}



#endif
