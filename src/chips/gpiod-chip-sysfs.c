#include "gpiod-chip-sysfs.h"
#include "gpiod-chip.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

const char* get_value(int fd, const char* name, const char* file)
{
    int ret = 0;
    int errsv = 0;
    static char buffer[256];

    sprintf(buffer, "%s/%s", name, file);

    int value_fd = openat(fd, buffer, O_RDONLY);
    errsv = errno;

    if(value_fd == -1)
        goto fail;


    ret = read(value_fd, &buffer, sizeof(buffer));
    errsv = errno;

    if(ret <= 0)
        goto fail_close;

    buffer[ret] = '\0';

    close(value_fd);

    return buffer;

    fail_close:
    close(value_fd);

    fail:
    errno = errsv;
    return 0;
}

struct gpio_chip* sysfs_init_gpio_chip(int fd, const char* name)
{
    static const char sbase[] = "base";
    static const char sngpio[] = "ngpio";
    static const char slabel[] = "label";

    struct gpio_chip* chip = 0;

    int ret = 0;
    int errsv = 0;
    const char* value = 0;

    value = get_value(fd, name, sbase);
    int base = atoi(value);

    value = get_value(fd, name, sngpio);
    int ngpio = atoi(value);

    chip = malloc(sizeof(struct gpio_chip));
    chip->base = base;
    chip->ngpio = ngpio;

    value = get_value(fd, name, slabel);

    strncpy(chip->label, value, sizeof(chip->label));
    ret = strlen(chip->label);
    chip->label[ret - 1] = '\0';

    chip->fd = fd;

    return chip;

    fail:
    errno = errsv;
    return 0;
}
