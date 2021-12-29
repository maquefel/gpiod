#include "gpiod-sysfs.h"
#include "gpiod-pin.h"

#include "list.h"

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char* sysfs = "/sys/class/gpio/";

static int sfd_ = -1; // /sys/class/gpio
static int efd_= - 1; // /sys/class/gpio/export
static int ufd_ = -1; // /sys/class/gpio/unexport

static DIR* dir;

struct gpio_pin_sysfs* to_pin_sysfs(struct gpio_pin* pin)
{
    return container_of(pin, struct gpio_pin_sysfs, pin);
}

struct gpio_pin* alloc_sysfs_gpio_pin()
{
    struct gpio_pin_sysfs* gp = malloc(sizeof(struct gpio_pin_sysfs));
    gp->pin.facility = GPIOD_FACILITY_SYSFS;
    gp->pin.init = init_sysfs_pin;
    gp->pin.cleanup = cleanup_sysfs_pin;

    gp->pin.changed = sysfs_changed_value;

    gp->active_fd = 0;
    gp->dir_fd = 0;
    gp->edge_fd = 0;
    gp->value_fd = 0;

    return &gp->pin;
}

int init_sysfs()
{
    int errsv = 0;

    dir = opendir(sysfs);
    errsv = errno;
    if(!dir) {
        syslog(LOG_CRIT, "failed opening /sys/class/gpio with [%d] : %s", errsv, strerror(errsv));
        goto fail;
    }

    sfd_ = dirfd(dir);
    errsv = errno;

    if(sfd_ == -1) {
        syslog(LOG_CRIT, "failed dirfd for /sys/class/gpio with [%d] : %s", errsv, strerror(errsv));
        goto fail;
    }

    efd_ = openat(sfd_, "export", O_WRONLY | O_CLOEXEC);
    errsv = errno;
    if(efd_ == -1) {
        syslog(LOG_CRIT, "failed opening /sys/class/gpio/export with [%d] : %s", errsv, strerror(errsv));
        goto fail;
    }

    ufd_ = openat(sfd_, "unexport", O_WRONLY | O_CLOEXEC);
    errsv = errno;
    if(ufd_ == -1) {
        syslog(LOG_CRIT, "failed opening /sys/class/gpio/unexport with [%d] : %s", errsv, strerror(errsv));
        goto fail;
    }

    return 0;

    fail:
    errno = errsv;
    return -1;
}

void cleanup_sysfs()
{
    close(sfd_);
    close(efd_);
    close(ufd_);
    closedir(dir);
}

int sysfs_set_edge(int fd, enum GPIOD_EDGE edge)
{
    int ret = 0;
    int errsv = 0;

    switch(edge) {
        case GPIOD_EDGE_RISING:
        case GPIOD_EDGE_FALLING:
        case GPIOD_EDGE_BOTH:
            ret = write(fd, gpiod_edge_array[edge], strlen(gpiod_edge_array[edge]));
            errsv = errno;
            break;
        case GPIOD_EDGE_POLLED:
            ret = write(fd, "none", strlen("none"));
            errsv = errno;
            break;
        default:
            return -1;
    }

    if(ret > 0)
        return 0;

    errno = errsv;
    return -1;
}

int sysfs_set_direction(int fd) /** well we need in and always in */
{
    int ret = 0;
    int errsv = 0;
    ret = write(fd, "in", strlen("in"));
    errsv = errno;
    if(ret > 0)
        return 0;

    errno = errsv;
    return -1;
}

int sysfs_set_active_low(int fd, enum GPIOD_ACTIVE_LOW a_low)
{
    int ret = 0;
    int errsv = 0;

    switch(a_low) {
        case GPIOD_ACTIVE_YES:
            ret = write(fd, "1", 1);
            errsv = errno;
            break;
        case GPIOD_ACTIVE_NO:
            ret = write(fd, "0", 1);
            errsv = errno;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    if(ret > 0)
        return 0;

    errno = errsv;
    return -1;
}

int8_t sysfs_changed_value(struct gpio_pin* pin, struct timespec* time, int8_t* event)
{
    char value = 0;
    int errsv = 0;

    value = sysfs_read_value(pin);
    errsv = errno;

    if(value < 0)
        goto fail;

    if(value == pin->value_)
        return 0;

    if(value == 1)
        *event = GPIOD_EDGE_RISING;
    else
        *event = GPIOD_EDGE_FALLING;

    clock_gettime(CLOCK_REALTIME, time);

    pin->value_ = value;
    return 1;

    fail:
    errno = errsv;
    return -1;
}

int8_t sysfs_read_value(struct gpio_pin* pin)
{
    int ret = 0;
    int errsv = 0;
    char value = -1;

    struct gpio_pin_sysfs* spin = to_pin_sysfs(pin);

    ret = lseek(spin->value_fd, 0, SEEK_SET);
    errsv = errno;

    if(ret == -1)
        goto fail;


    ret = read(spin->value_fd, &value, 1);
    errsv = errno;

    if(ret <= 0)
        goto fail;

    return value - 0x30;

    fail:
    errno = errsv;
    return -1;
}

int init_sysfs_pin(struct gpio_pin* pin)
{
    int ret = 0;
    int errsv = 0;
    int gpio_dir = -1;

    char buffer[64] = {0};

    struct gpio_pin_sysfs* spin = to_pin_sysfs(pin);

    size_t n = sprintf(buffer, "%d", pin->system);
    ret = write(efd_, buffer, n);
    errsv = errno;

    if(ret < 0 && errno != EBUSY) { /** EBUSY === EXISTS */
        syslog(LOG_CRIT, "pin export %s failed with [%d] : %s", buffer, errsv, strerror(errsv));
        goto fail;
    }

    if(pin->name[0] != '\0') {
        sprintf(buffer, "%s", pin->name);
    } else {
        sprintf(buffer, "gpio%d", pin->system);
    }

    gpio_dir = openat(sfd_, buffer, O_RDONLY | O_CLOEXEC);
    errsv = errno;

    if(gpio_dir == -1) {
        syslog(LOG_CRIT, "failed opening %s with [%d] : %s", buffer, errsv, strerror(errsv));
        goto fail;
    }

    ret = spin->dir_fd = openat(gpio_dir, "direction", O_WRONLY | O_CLOEXEC);
    errsv = errno;

    if(ret > 0) {
        ret = sysfs_set_direction(ret);
        if(ret == -1) {
            syslog(LOG_CRIT, "failed setting direction to `in` with [%d] : %s", errsv, strerror(errsv));
            goto fail;
        }
    } else {
        syslog(LOG_CRIT, "failed opening direction with [%d] : %s", errsv, strerror(errsv));
        goto fail;
    }

    ret = spin->edge_fd = openat(gpio_dir, "edge", O_WRONLY | O_CLOEXEC);
    errsv = errno;

    if(ret > 0) {
        ret = sysfs_set_edge(ret, pin->edge);
        errsv = errno;
        if(ret == -1)
            syslog(LOG_WARNING, "failed setting edge with [%d] : %s", errsv, strerror(errsv));
    } else {
        syslog(LOG_WARNING, "failed opening edge file with [%d] : %s", errsv, strerror(errsv));
    }

    if(pin->active_low == GPIOD_ACTIVE_YES) {
        ret = spin->active_fd = openat(gpio_dir, "active_low", O_WRONLY | O_CLOEXEC);
        errsv = errno;
        if(ret > 0) {
            ret = sysfs_set_active_low(ret, GPIOD_ACTIVE_YES);
            errsv = errno;
            if(ret == -1)
                syslog(LOG_WARNING, "failed setting active_low with [%d] : %s", errsv, strerror(errsv));
        } else {
            syslog(LOG_WARNING, "failed opening active_fd with [%d] : %s", errsv, strerror(errsv));
        }
    }

    ret = spin->value_fd = openat(gpio_dir, "value", O_RDONLY | O_CLOEXEC);
    errsv = errno;

    if(ret < 0) {
        syslog(LOG_CRIT, "failed opening `value` with [%d] : %s", errsv, strerror(errsv));
        goto fail;
    }

    /** read initial value */
    pin->value_ = sysfs_read_value(pin);
    pin->fd = spin->value_fd;

    close(gpio_dir);

    return 0;

    fail:
    errno = errsv;
    return -1;
}

int cleanup_sysfs_pin(struct gpio_pin* pin)
{
    struct gpio_pin_sysfs* spin = to_pin_sysfs(pin);
    int ret = 0;
    int errsv = 0;

    char buffer[64] = {0};

    size_t n = sprintf(buffer, "%d", pin->system);
    ret = write(ufd_, buffer, n);
    errsv = errno;

    if(ret <= 0) {
        syslog(LOG_CRIT, "pin unexport %s failed with [%d] : %s", buffer, errsv, strerror(errsv));
    }

    if(spin->active_fd) {
        if(pin->active_low == GPIOD_ACTIVE_YES)
        sysfs_set_active_low(ret, GPIOD_ACTIVE_NO);
        close(spin->active_fd);
        spin->active_fd = -1;
    }

    if(spin->value_fd) {close(spin->value_fd); spin->value_fd = -1;}
    if(spin->edge_fd) {close(spin->edge_fd); spin->edge_fd = -1;}
    if(spin->dir_fd) {close(spin->dir_fd); spin->dir_fd = -1;}

    return 0;
}
