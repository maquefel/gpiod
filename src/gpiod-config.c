/** @file gpiod-config.c
*
* @brief Main file for the gpio daemon
*
* @par
*/

#include <confuse.h>
#include <string.h>
#include <syslog.h>
#include <stdint.h>
#include <errno.h>
#include <arpa/inet.h>

#include "gpiod-pin.h"
#include "gpiod-server.h"

int conf_parse_facility(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
    int i;
    for(i = 0; i < GPIOD_FACILITY_MAX; i++)
        if(strncmp(gpiod_facility_array[i], value, strlen(gpiod_facility_array[i])) == 0) {
            *(int *)result = i;
            return 0;
        }

    cfg_error(cfg, "invalid value for option '%s': %s", cfg_opt_name(opt), value);

    return -1;
}

int conf_parse_edge(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
    int i;
    for(i = 0; i < GPIOD_EDGE_MAX; i++)
        if(strncmp(gpiod_edge_array[i], value, strlen(gpiod_edge_array[i])) == 0) {
            *(int *)result = i;
            return 0;
        }

    cfg_error(cfg, "invalid value for option '%s': %s", cfg_opt_name(opt), value);

    return -1;
}

cfg_opt_t gpio_pin_opts[] = {
    CFG_INT_CB("facility", GPIOD_FACILITY_MAX, CFGF_NONE, conf_parse_facility),
    CFG_INT("system", -1, CFGF_NONE),
    CFG_STR("gpiochip", 0, CFGF_NONE),
    CFG_INT("offset", -1, CFGF_NONE),
    CFG_INT("local", -1, CFGF_NONE),
    CFG_STR("label", 0, CFGF_NODEFAULT),
    CFG_INT_CB("edge", GPIOD_EDGE_MAX, CFGF_NONE, conf_parse_edge),
    CFG_BOOL("active_low", cfg_false, CFGF_NONE),
    CFG_END()
};

/* validates an option (must be positive) */
int conf_validate_positive(cfg_t *cfg, cfg_opt_t *opt)
{
    int value = cfg_opt_getnint(opt, 0);

    if (value < 0) {
        cfg_error(cfg, "invalid %s in section '%s' should be positive", cfg_opt_name(opt), cfg_name(cfg));
        return -1;
    }

    return 0;
}

cfg_opt_t opts[] = {
    CFG_STR("listen", 0, CFGF_NONE),
    CFG_INT("port", 0, CFGF_NONE),
    CFG_INT("poll", 0, CFGF_NONE),
    CFG_INT_CB("facility", GPIOD_FACILITY_SYSFS, CFGF_NONE, conf_parse_facility),
    CFG_SEC("gpio", gpio_pin_opts, CFGF_MULTI),
    CFG_END()
};

int loadConfig(const char* fileName) /** bool reload*/
{
    cfg_t *cfg = 0;

    int ret = 0;
    int errsv = 0;
    char *ip = 0;
    int n = 0;

    cfg = cfg_init(opts, CFGF_NONE);
    cfg_set_validate_func(cfg, "port", conf_validate_positive);
    cfg_set_validate_func(cfg, "poll", conf_validate_positive);
    cfg_set_validate_func(cfg, "gpio|offset", conf_validate_positive);
    cfg_set_validate_func(cfg, "gpio|system", conf_validate_positive);
    cfg_set_validate_func(cfg, "gpio|local", conf_validate_positive);

    ret = cfg_parse(cfg, fileName);

    switch(ret) {
        case CFG_FILE_ERROR:
            syslog(LOG_CRIT, "Couldn't oped file: %s for reading", fileName);
            ret = CFG_FILE_ERROR;
            goto parse_fail;
        case CFG_PARSE_ERROR:
            syslog(LOG_CRIT, "Error parsing file: %s", fileName);
            ret = CFG_PARSE_ERROR;
            goto parse_fail;
        case CFG_SUCCESS:
            debug_printf_n("file %s open succefully.", fileName);
            break;
    }

    port = cfg_getint(cfg, "port");
    ip = cfg_getstr(cfg, "listen");

    ret = inet_pton(PF_INET, ip, &addr);
    errsv = errno;

    if(ret == 0) {
        syslog(LOG_CRIT, "Error parsing file %s invalid ip address provided: %s", fileName, ip);
        goto parse_fail_free;
    }

    if(ret == -1) {
        syslog(LOG_CRIT, "Error parsing file %s invalid ip address provided: %s - %s", fileName, ip, strerror(errsv));
    }

    enum GPIOD_FACILITY global_facility = cfg_getint(cfg, "facility");

    n = cfg_size(cfg, "gpio");

    for(int i = 0; i < n; i++)
    {
        cfg_t* gp = cfg_getnsec(cfg, "gpio", i);
        enum GPIOD_FACILITY facility = cfg_getint(gp, "facility");

        if(facility == GPIOD_FACILITY_MAX) {
            syslog(LOG_INFO, "no facility provided at %d, using global facility : %s", gp->line, gpiod_facility_array[global_facility]);
            facility = global_facility;
        }

        struct gpio_pin* pin = 0;

        pin = alloc_gpio_pin(facility);

        if(pin == 0) {
            syslog(LOG_ERR, "failed to allocate pin: %s type at %d line", gpiod_facility_array[facility], gp->line);
            debug_printf_n("failed to allocate pin: %s type at %d line", gpiod_facility_array[facility], gp->line);
            continue;
        }

        pin->system = cfg_getint(gp, "system");
        char* chip = cfg_getstr(gp, "gpiochip");
        if(chip)
            strncpy(pin->gpiochip, chip, sizeof(pin->gpiochip));

        pin->offset = cfg_getint(gp, "offset");
        pin->local = cfg_getint(gp, "local");

        char* label = cfg_getstr(gp, "label");
        if(label)
            strncpy(pin->label, label, sizeof(pin->label));

        pin->edge = cfg_getint(gp, "edge");

        pin->active_low = (enum GPIOD_ACTIVE_LOW)cfg_getbool(gp, "active_low");

        list_add_tail(&(pin->list), &(gp_list));
    }

    parse_fail_free:
    cfg_free(cfg);
    parse_fail:
    return ret;
}
