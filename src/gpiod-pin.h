#ifndef __GPIOD_PIN_H__
#define __GPIOD_PIN_H__

#include "gpiod.h"
#include "list.h"

#include "gpiod-hooktab.h"

enum GPIOD_FACILITY {
    GPIOD_FACILITY_SYSFS = 0,
    GPIOD_FACILITY_UAPI = 1,
    GPIOD_FACILITY_MAX
};

extern const char* gpiod_facility_array[GPIOD_FACILITY_MAX];

enum GPIOD_EDGE {
    GPIOD_EDGE_RISING = 0,
    GPIOD_EDGE_FALLING,
    GPIOD_EDGE_BOTH,
    GPIOD_EDGE_POLLED,
    GPIOD_EDGE_MAX
};

extern const char* gpiod_edge_array[GPIOD_EDGE_MAX];

enum GPIOD_ACTIVE_LOW {
    GPIOD_ACTIVE_NO = 0,
    GPIOD_ACTIVE_YES = 1,
    GPIOD_ACTIVE_MAX
};

struct gpio_pin;

struct gpio_pin* alloc_gpio_pin(enum GPIOD_FACILITY /*facility*/);

typedef int (*init_pin)(struct gpio_pin*);
typedef int (*cleanup_pin)(struct gpio_pin*);

typedef char (*changed_pin)(struct gpio_pin*);
typedef int (*value_pin)(struct gpio_pin*);

// system or gpiochip + offset; system is translated to gpiochip + offset; gpiochip + offset is translated to system
struct gpio_pin {
    int fd; ///> equal to /sys/class/gpio/gpioN/value for sysfs

    int system;

    char gpiochip[32];
    int offset;

    int local;

    char name[32]; ///> gpio line name, obligatorily if gpio line was named via device tree or platform driver, otherwise we can't uniquely identify exported line name
    char label[32];

    init_pin init;
    cleanup_pin cleanup;

    changed_pin changed;
    value_pin value;

    char value_;

    enum GPIOD_FACILITY facility;
    enum GPIOD_EDGE edge;
    enum GPIOD_ACTIVE_LOW active_low;

    struct list_head list;
    struct list_head hook_list;
};

struct gpio_pin_sysfs {
    struct gpio_pin pin;
    int active_fd;
    int dir_fd;
    int edge_fd;
    int value_fd;
};

struct gpio_pin_uapi {
    struct gpio_pin pin;
    int fd;
    struct gpio_chip* chip;
};

extern struct list_head gp_list;

struct gpio_pin_sysfs* to_pin_sysfs(struct gpio_pin* /*pin*/);
struct gpio_pin_uapi* to_pin_uapi(struct gpio_pin* /*pin*/);

struct gpio_pin* alloc_sysfs_gpio_pin();
struct gpio_pin* alloc_uapi_gpio_pin();

int init_gpio_pins();
int cleanup_gpio_pins();
int free_gpio_pins();

void free_gpio_pin(struct gpio_pin*);

struct gpio_pin* find_pin_by_label(const char* /*label*/);

#endif
