#ifndef __GPIOD_CRON_H__
#define __GPIOD_CRON_H__

#include <sys/types.h>
#include <stdint.h>

#include "list.h"

#ifndef GPIOD_TAB_MAX_ARGUMENT_LENGTH
#define GPIOD_TAB_MAX_ARGUMENT_LENGTH 128
#endif

enum GPIOD_HOOKTAB_MOD {
    HOOKTAB_MOD_EMPTY = 0,
    HOOKTAB_MOD_RISE = 1UL << 0,
    HOOKTAB_MOD_FALL = 1UL << 1,
    HOOKTAB_MOD_BOTH = HOOKTAB_MOD_RISE | HOOKTAB_MOD_FALL,
    HOOKTAB_MOD_NO_LOOP = 1UL << 2,
    HOOKTAB_MOD_ONESHOT= 1UL << 3,
    HOOKTAB_MOD_MAX = 0xff
};

enum GPIOD_HOOKTAB_ARGS {
    HOOKTAB_ARG_LABEL = 0,
    HOOKTAB_ARG_EVENT_TEXT,
    HOOKTAB_ARG_EVENT_NUM,
    HOOKTAB_ARG_MAX
};

struct gpiod_hook_args {
    enum GPIOD_HOOKTAB_ARGS arg;
    char* c_arg;
    struct list_head list;
};

struct gpiod_hook {
    char* path;                 ///> path to executable
    uint16_t flags;             ///> reaction flags -> GPIOD_HOOKTAB_MOD
    int8_t fired;               ///> hook was fired at least one time
    pid_t spawned;              ///> pid of the spawned proccess hook
    uint8_t arg_list_size;      ///> size of proccessed argument list
    struct list_head list;      ///>
    struct list_head arg_list;  ///> argument list
};

int loadTabs(const char* /*tabsDir*/);

struct gpio_pin;
void freeTabs(const struct gpio_pin* /*pin*/);

#endif
