#ifndef __GPIOD_SYSFS_H__
#define __GPIOD_SYSFS_H__

int init_sysfs();
void cleanup_sysfs();

struct gpio_pin;

struct gpio_pin* alloc_sysfs_gpio_pin();
int init_sysfs_pin(struct gpio_pin* /*pin*/);
int cleanup_sysfs_pin(struct gpio_pin* /*pin*/);
char sysfs_read_value(struct gpio_pin* /*pin*/);
char sysfs_changed_value(struct gpio_pin* /*pin*/);

#endif
