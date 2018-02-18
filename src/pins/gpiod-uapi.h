#ifndef __GPIOD_UAPI_H__
#define __GPIOD_UAPI_H__

struct gpio_pin;

int uapi_init_pin(struct gpio_pin* /*pin*/);
int uapi_cleanup_pin(struct gpio_pin* /*pin*/);

char uapi_changed_value(struct gpio_pin* /*pin*/);
char uapi_read_value(struct gpio_pin* /*pin*/);

#endif
