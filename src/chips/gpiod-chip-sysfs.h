#ifndef __GPIOD_CHIP_SYSFS_H__
#define __GPIOD_CHIP_SYSFS_H__

struct gpio_chip;

struct gpio_chip* sysfs_init_gpio_chip(int /*fd*/, const char* /*name*/);

#endif
