#include "gpiod-pin.h"

#ifdef GPIOD_SYSFS
#include "pins/gpiod-sysfs.h"
#endif

#ifdef GPIOD_UAPI
#include "pins/gpiod-uapi.h"
#endif

#include <stdlib.h>

const char* gpiod_facility_array[GPIOD_FACILITY_MAX] = {
    "sysfs",
    "uapi",
};

const char* gpiod_edge_array[GPIOD_EDGE_MAX] = {
    "rising",
    "falling",
    "both",
    "polled",
};

struct list_head gp_list = {0};

struct gpio_pin* alloc_gpio_pin(enum GPIOD_FACILITY facility)
{
    struct gpio_pin* pin = 0;


    switch(facility) {
        case GPIOD_FACILITY_SYSFS:
            pin = alloc_sysfs_gpio_pin();
        break;
#ifdef GPIOD_UAPI
        case GPIOD_FACILITY_UAPI:
            pin = alloc_uapi_gpio_pin();
        break;
#endif
        default:
            break;
    }

    return pin;
}

int init_gpio_pins()
{
    struct list_head* pos = 0;
    struct list_head* tmp = 0;
    struct gpio_pin* pin = 0;
    int ret = 0;

    list_for_each_safe(pos, tmp, &gp_list) {
        pin = list_entry(pos, struct gpio_pin, list);
        ret = pin->init(pin);
        if(ret == -1) {
            list_del(&pin->list);
            free_gpio_pin(pin);
        }
    }

    return 0;
}

int cleanup_gpio_pins()
{
    struct list_head* pos = 0;
    struct gpio_pin* pin = 0;
    int ret = 0;

    list_for_each(pos, &gp_list) {
        pin = list_entry(pos, struct gpio_pin, list);
        ret = pin->cleanup(pin);
    }

    return 0;
}

int free_gpio_pins()
{
    struct list_head* pos = 0;
    struct list_head* tmp = 0;
    struct gpio_pin* pin = 0;

    list_for_each_safe(pos, tmp, &gp_list) {
        pin = list_entry(pos, struct gpio_pin, list);
        free_gpio_pin(pin);
    }

    return 0;
}

void free_gpio_pin(struct gpio_pin* pin)
{
    switch(pin->facility) {
        case GPIOD_FACILITY_SYSFS: {
            struct gpio_pin_sysfs* gp = to_pin_sysfs(pin);
            free(gp);
        } break;
#ifdef GPIOD_UAPI
        case GPIOD_FACILITY_UAPI: {
            struct gpio_pin_uapi* gp = to_pin_uapi(pin);
            free(gp);
        } break;
#endif
        default:
            break;
    }
}