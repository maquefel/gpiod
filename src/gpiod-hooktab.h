#ifndef __GPIOD_CRON_H__
#define __GPIOD_CRON_H__

#include <stdint.h>

#include "list.h"

#ifndef GPIOD_TAB_MAX_ARGUMENT_LENGTH
#define GPIOD_TAB_MAX_ARGUMENT_LENGTH 128
#endif

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
    uint16_t flags;             ///> reaction flags GE_EDGE_RISING | GE_EDGE_FALLING | GE_ONESHOT
    uint8_t arg_list_size;      ///> size of proccessed argument list
    struct list_head list;      ///>
    struct list_head arg_list;  ///> argument list
};

int loadTabs(const char* /*tabsDir*/);

struct gpio_pin;
int freeTabs(const struct gpio_pin* /*pin*/);

#endif
