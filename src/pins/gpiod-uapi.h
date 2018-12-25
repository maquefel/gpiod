#ifndef __GPIOD_UAPI_H__
#define __GPIOD_UAPI_H__

#include <stdint.h>

struct gpio_pin;

int uapi_init_pin(struct gpio_pin* /*pin*/);
int uapi_cleanup_pin(struct gpio_pin* /*pin*/);

int8_t uapi_changed_value(struct gpio_pin* /*pin*/);
int8_t uapi_read_value(struct gpio_pin* /*pin*/);

#endif
