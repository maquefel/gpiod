#ifndef __GPIOD_DISPATCH_H__
#define __GPIOD_DISPATCH_H__

#include <unistd.h>
#include <stdint.h>
#include <time.h>

int dispatch(struct timespec /*ts*/, uint32_t /*chan*/, uint8_t /*value*/);
struct gpio_pin;
int dispatch_hooks(struct gpio_pin* /*pin*/, int8_t /*event*/);
int hook_clear_spawned(pid_t /*pid*/);
int splice_to(int /*from_fd*/, int /*to_fd*/, ssize_t /*num*/);
int splice_to_null(int /*from_fd*/, ssize_t /*num*/);

#endif
