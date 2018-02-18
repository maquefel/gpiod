#ifndef __GPIOD_CHIP_UAPI_H__
#define __GPIOD_CHIP_UAPI_H__

struct gpio_chip;

struct gpio_chip* uapi_init_gpio_chip(int /*fd*/, const char* /*name*/);

#endif
