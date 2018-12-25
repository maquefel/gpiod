#ifndef __GPIOD_EXEC_H__
#define __GPIOD_EXEC_H__

int exec_start(const char* /*exec_file*/, char *const /*argv*/[], char *const /*envp*/[]);
int exec_stop(pid_t /*pid*/);
int exec_kill(pid_t /*pid*/);

#endif
