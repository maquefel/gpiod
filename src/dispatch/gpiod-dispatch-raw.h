#ifndef __GPIOD_DISPATCH_RAW_H__
#define __GPIOD_DISPATCH_RAW_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

size_t dispatch_raw_pack(char** /*data*/, struct timeval /*ts*/, uint8_t /*chan*/, uint8_t /*value*/);

#endif
