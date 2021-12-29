#include "gpiod-uapi.h"
#include "gpiod-pin.h"
#include "gpiod-chip.h"
#include "gpiod-server.h"

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include <syslog.h>
#include <time.h>

struct gpio_pin_uapi* to_pin_uapi(struct gpio_pin* pin)
{
    return container_of(pin, struct gpio_pin_uapi, pin);
}

struct gpio_pin* alloc_uapi_gpio_pin()
{
    struct gpio_pin_uapi* gp = malloc(sizeof(struct gpio_pin_uapi));
    gp->pin.facility = GPIOD_FACILITY_UAPI;
    gp->pin.init = uapi_init_pin;
    gp->pin.cleanup = uapi_cleanup_pin;

    gp->pin.changed = uapi_changed_value;

    return &gp->pin;
}

int uapi_init_pin(struct gpio_pin* pin)
{
    int errsv = 0;
    int ret = 0;
    struct gpioevent_request req;
    struct gpio_pin_uapi* upin = 0;

    if(!strlen(pin->gpiochip)) {
        errsv = EINVAL;
        goto fail;
    }

    struct gpio_chip* chip = get_chip(pin->gpiochip);

    if(!chip) {
        syslog(LOG_CRIT, "no chip: %s found for %d:%s", pin->gpiochip, pin->local, pin->label);
        errsv = EINVAL;
        goto fail;
    }

    req.lineoffset = pin->local;
    req.handleflags = GPIOHANDLE_REQUEST_INPUT;

    if(pin->active_low)
        req.handleflags |= GPIOHANDLE_REQUEST_ACTIVE_LOW;

    switch(pin->edge) {
        case GPIOD_EDGE_RISING:
            req.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;
            break;
        case GPIOD_EDGE_FALLING:
            req.eventflags = GPIOEVENT_REQUEST_FALLING_EDGE;
            break;
        case GPIOD_EDGE_BOTH:
            req.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
            break;
        case GPIOD_EDGE_POLLED:
        default:
            req.eventflags = 0;
            break;
    }

    /** @todo add prefix gpiod to consumer label */
    strcpy(req.consumer_label, pin->label);

    ret = ioctl(chip->fd, GPIO_GET_LINEEVENT_IOCTL, &req);
    errsv = errno;

    if(ret == -1)
        goto fail;

    upin = to_pin_uapi(pin);

    upin->chip = chip;
    upin->fd = req.fd;

    pin->value_ = uapi_read_value(pin);
    pin->fd = req.fd;

    make_non_blocking(req.fd);

    return 0;

    fail:
    errno = errsv;
    return -1;
}

int uapi_cleanup_pin(struct gpio_pin* pin)
{
    struct gpio_pin_uapi* upin = to_pin_uapi(pin);
    /** close handle */
    close(upin->fd);

    return 0;
}

int8_t uapi_read_value(struct gpio_pin* pin)
{
    int ret = 0;
    int errsv = 0;
    struct gpiohandle_data data;

    struct gpio_pin_uapi* upin = to_pin_uapi(pin);

    ret = ioctl(upin->fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
    errsv = errno;

    if (ret == -1)
        goto fail;

    return data.values[0];

    fail:
    errno = errsv;
    return -1;
}

/** TODO: split polled and not polled version */
int8_t uapi_changed_value(struct gpio_pin* pin, struct timespec* time, int8_t* event)
{
    int ret = 0;
    char value = 0;
    int errsv = 0;

    struct gpioevent_data ev;
    struct gpio_pin_uapi* upin = to_pin_uapi(pin);

    if(pin->edge != GPIOD_EDGE_POLLED) {
        ret = read(upin->fd, &ev, sizeof(ev));
        errsv = errno;

        syslog(LOG_DEBUG, "read returned %d", ret);
        debug_printf_n("read returned %d", ret);

        if(ret == -1)
            goto fail;

        switch(ev.id) {
            case GPIOEVENT_EVENT_RISING_EDGE:
                pin->value_ = 1;
                *event = GPIOD_EDGE_RISING;
                break;
            case GPIOEVENT_EVENT_FALLING_EDGE:
                pin->value_ = 0;
                *event = GPIOD_EDGE_FALLING;
                break;
            default:
                errsv = ENOENT;
                goto fail;

        }

        time->tv_sec = ev.timestamp/(uint64_t)1e9;
        time->tv_nsec = ev.timestamp%(uint64_t)1e9;

        return 1;
    }

    value = uapi_read_value(pin);
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
