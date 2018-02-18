#ifndef __GPIOD_CHIP_H__
#define __GPIOD_CHIP_H__

#include <stdint.h>
#include "uthash.h"

struct gpio_chip {
    uint16_t base;
    uint16_t ngpio;
    char label[32];
    char name[32];

    int fd;

    UT_hash_handle hh;
};

struct gpio_pin;
struct gpio_chip* chip_from_pin(struct gpio_pin* /* pin */);

struct gpio_chip* get_chip(const char* /*name*/);

int init_gpio_chips();

int init_gpio_chips_sysfs();
void cleanup_gpio_chips();

#endif
