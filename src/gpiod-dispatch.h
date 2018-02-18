#ifndef __GPIOD_DISPATCH_H__
#define __GPIOD_DISPATCH_H__

#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

int dispatch(struct timeval /*ts*/, uint32_t /*chan*/, uint8_t /*value*/);
int splice_to(int /*from_fd*/, int /*to_fd*/, ssize_t /*num*/);
int splice_to_null(int /*from_fd*/, ssize_t /*num*/);

#endif
