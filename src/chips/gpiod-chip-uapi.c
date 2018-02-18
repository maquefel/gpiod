#include "gpiod-chip-uapi.h"

#include <linux/gpio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "gpiod-chip.h"

struct gpio_chip* uapi_init_gpio_chip(int fd, const char* name)
{
    int ret = 0;
    int errsv = 0;
    struct gpio_chip* chip = 0;
    struct gpiochip_info chip_info = {0};

    int chip_fd = openat(fd, name, O_RDONLY);
    errsv = errno;

    if(chip_fd < 0)
        goto fail;

    ret = ioctl(chip_fd, GPIO_GET_CHIPINFO_IOCTL, &chip_info);
    errsv = errno;

    if (ret == -1)
        goto fail;

    chip = malloc(sizeof(struct gpio_chip));

    chip->base = 0;
    chip->ngpio = chip_info.lines;

    strncpy(chip->label, chip_info.label, sizeof(chip->label));
    strncpy(chip->name, chip_info.name, sizeof(chip->name));

    chip->fd = chip_fd;

    return chip;

    fail:
    errno = errsv;
    return 0;
}
