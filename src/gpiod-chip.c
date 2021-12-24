#include "gpiod-chip.h"

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "gpiod-pin.h"
#include "gpiod.h"
#include "list.h"

#include "chips/gpiod-chip-sysfs.h"

typedef struct gpio_chip* (*init_chip_func)(int, const char*);

init_chip_func init_chip;

#ifdef GPIOD_UAPI
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include "chips/gpiod-chip-uapi.h"
#define GPIOCHIP_DIR "/dev"
#else
#define GPIOCHIP_DIR "/sys/class/gpio"
#endif

struct gpio_chip* stored_chips = NULL;

struct gpio_chip* sysfs_chip_from_system(int system)
{
    return 0;
}

struct gpio_chip* uapi_chip_from_system(int system)
{
    return 0;
}

struct gpio_chip* sysfs_chip_from_name(const char* name)
{
    return 0;
}

struct gpio_chip* uapi_chip_from_name(const char* name)
{
    return 0;
}

struct gpio_chip* chip_from_pin(struct gpio_pin* pin)
{
    /** chip from system */

    /** chip from name */

    return 0;
}

int init_gpio_chips()
{
    /** scan gpiochips */
    const struct dirent *ent;
    DIR *dp;
    int dir_fd;
    int errsv = 0;

#ifdef GPIOD_UAPI
    init_chip = uapi_init_gpio_chip;
#else
    init_chip = sysfs_init_gpio_chip;
#endif

    /* List all GPIO devices one at a time */
    dp = opendir(GPIOCHIP_DIR);
    if (!dp) {
        errsv = errno;
        goto fail;
    }

    dir_fd = dirfd(dp);

    while (ent = readdir(dp), ent) {
        if (check_prefix(ent->d_name, "gpiochip")) {
            struct gpio_chip* chip = init_chip(dir_fd, ent->d_name);
            if (!chip)
                break;

            HASH_ADD_STR(stored_chips, name, chip);
        }
    }

    closedir(dp);

    return 0;

    fail:
    errno = errsv;
    return -1;
}

void cleanup_gpio_chips()
{
    struct gpio_chip *chip, *tmp;

    HASH_ITER(hh, stored_chips, chip, tmp)
    {
        HASH_DEL(stored_chips, chip);
        close(chip->fd);
        free(chip);
    }
}

struct gpio_chip* get_chip(const char* name)
{
    struct gpio_chip *chip = 0;
    HASH_FIND_STR(stored_chips, name, chip);
    return chip;
}
