#ifndef __GPIOD_H__
#define __GPIOD_H__

#ifndef MAJOR
#define MAJOR 0
#endif

#ifndef MINOR
#define MINOR 0
#endif

#ifndef VERSION_PATCH
#define VERSION_PATCH 0
#endif

#define PACKAGE_VERSION MAJOR "." MINOR "." VERSION_PATCH

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "gpiod"
#endif

#ifndef GPIOD_PID_FILE
#define GPIOD_PID_FILE "/var/run/" PACKAGE_NAME ".pid"
#endif

#ifndef GPIOD_CONFIG_FILE_PATH
#define GPIOD_CONFIG_FILE_PATH "/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".conf"
#endif

#ifndef GPIOD_CRON_TABLE_DIRECTORY
#define GPIOD_CRON_TABLE_DIRECTORY "/etc/" PACKAGE_NAME ".d/"
#endif

#define GPIOD_SYSFS

#ifdef WITHOUT_GPIOD_SYSFS
#undef GPIOD_SYSFS
#endif

#define GPIOD_UAPI

#ifdef WITHOUT_GPIOD_UAPI
#undef GPIOD_UAPI
#endif

#define GPIOD_ERROR_CLOSING_FD          1
#define GPIOD_ERROR_OPENING_PID         2
#define GPIOD_ERROR_REGISTER_SIGNAL     3
#define GPIOD_ERROR_OPEN_CONFIG         4
#define GPIOD_ERROR_LOADING_CONFIG      5
#define GPIOD_ERROR_CONTROL_FIFO        6

#ifdef DEBUG
#include <stdio.h>
#define debug_printf(format, ...) printf("%s:%s:%d: " format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define debug_printf_n(format, ...) debug_printf(format "\n", ##__VA_ARGS__)
#else
#define debug_printf_n(format, args...) do{}while(0)
#endif

#include <stddef.h>

#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - offsetof(type,member) );})


#include <string.h>

static inline int check_prefix(const char *str, const char *prefix)
{
        return strlen(str) > strlen(prefix) &&
                strncmp(str, prefix, strlen(prefix)) == 0;
}

extern int devnullfd;

#endif
