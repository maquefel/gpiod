#include "gpiod-hooktab.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

#include "gpiod-pin.h"

const char* gpiod_event_array[GPIOD_EDGE_POLLED] = {
    "GPIOD_EDGE_RISING",
    "GPIOD_EDGE_FALLING",
    "GPIOD_EDGE_BOTH",
};

const char* gpiod_special_args_array[HOOKTAB_ARG_MAX] = {
    "$@",
    "$%",
    "$&"
};

enum GPIOD_EDGE tab_parse_event(const char* value)
{
    int i;
    for(i = 0; i < GPIOD_EDGE_POLLED; i++)
        if(strncmp(value, gpiod_event_array[i], strlen(value)) == 0)
            return (enum GPIOD_EDGE)i;

    return GPIOD_EDGE_MAX;
}

enum GPIOD_HOOKTAB_ARGS tab_parse_args(const char* value)
{
    int i;
    for(i = 0; i < HOOKTAB_ARG_MAX; i++)
        if(strncmp(value, gpiod_special_args_array[i], strlen(value)) == 0)
            return (enum GPIOD_HOOKTAB_ARGS)i;

    return HOOKTAB_ARG_MAX;
}

char* eat_space(const char* str)
{
    char* tmp = str;

    while(*tmp != '\0') {
        if(*tmp == 0x20 || *tmp == 0x09) {
            tmp++;
            continue;
        }

        break;
    }

    return tmp;
}

char* next_space(const char* str)
{
    char* tmp = str;

    while(*tmp != '\0') {
        if(*tmp == 0x20 || *tmp == 0x09 || *tmp == 0x0a || *tmp == 0x0d)
            break;

        tmp++;
    }

    return tmp;
}

int loadTab(int dirfd, struct dirent* dentry)
{
    int errsv = 0;
    int ret = 0;

    // label   event               exec path            args
    // WD0     GE_EDGE_RISING      /tmp/test.sh         $# $% $&
    // eat spaces and tabs
    /** open file */
    int fd = openat(dirfd, dentry->d_name, O_RDONLY);
    errsv = errno;
    if(fd == -1) {
        syslog(LOG_ERR, "Couldn't open file: %s for reading", dentry->d_name);
        goto fail;
    }

    FILE* file = fdopen(fd, "r");

    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    int8_t line_num = -1;

    while ((nread = getline(&line, &len, file)) != -1) {
        char buffer[GPIOD_TAB_MAX_ARGUMENT_LENGTH] = {0};
        char* tmp = 0;
        char* tmp1 = 0;
        char* tmp2 = 0;
        uint8_t len = 0;

        line_num++;

        /** get label */
        tmp1 = eat_space(line);
        tmp2 = next_space(tmp1);
        len = tmp2 - tmp1;
        tmp = strncpy(buffer, tmp1, len);

        /** find label */
        struct gpio_pin* pin = find_pin_by_label(tmp);
        if(pin == 0)
        {
            syslog(LOG_ERR, "line %d : label %s not found in gpio list", line_num, tmp);
            continue;
        }

        fprintf(stderr, "label : %s\n", tmp);

        /** get event */
        tmp1 = eat_space(tmp2);
        tmp2 = next_space(tmp1);
        len = tmp2 - tmp1;
        tmp = strncpy(buffer, tmp1, len);
        tmp[len] = '\0';

        enum GPIOD_EDGE event = tab_parse_event(tmp);

        if(event == GPIOD_EDGE_MAX)
        {
            syslog(LOG_ERR, "line %d : no known event %s", line_num, tmp);
            continue;
        }

        fprintf(stderr, "event : %s\n", tmp);

        /** get path */
        tmp1 = eat_space(tmp2);
        tmp2 = next_space(tmp1);
        len = tmp2 - tmp1;
        tmp = strncpy(buffer, tmp1, len);
        tmp[len] = '\0';

        /** allocate a hook here */
        struct gpiod_hook* hook = malloc(sizeof(struct gpiod_hook));
        hook->path = strdup(tmp);
        INIT_LIST_HEAD(&(hook->list));
        INIT_LIST_HEAD(&(hook->arg_list));
        hook->arg_list_size = 0;

        fprintf(stderr, "path : %s\n", hook->path);

        /** get all args */
//      $$ dollar sign ?
//      $@ watched input label
//      $% event flags (textually)
//      $& event flags (numerically)
        int i = 0;
        while(tmp1 != tmp2) {
            tmp1 = eat_space(tmp2);
            tmp2 = next_space(tmp1);
            len = tmp2 - tmp1;
            tmp = strncpy(buffer, tmp1, len);
            tmp[len] = '\0';
            fprintf(stderr, "arg[%d] : %s\n", i++, tmp);

            struct gpiod_hook_args* arg = malloc(sizeof(struct gpiod_hook_args));
            arg->arg = HOOKTAB_ARG_MAX;
            arg->c_arg = 0;
            INIT_LIST_HEAD(&arg->list);

            if(*tmp == '\0')
                goto list_add;

            /** parse args */
            if(*tmp == '$') {
                /** special args */
                enum GPIOD_HOOKTAB_ARGS crontab_arg = tab_parse_args(tmp);
                if(crontab_arg != HOOKTAB_ARG_MAX)
                {
                    arg->arg = crontab_arg;
                    goto list_add;

                }
            }

            arg->c_arg = strdup(tmp);

            list_add:
            list_add_tail(&(arg->list), &(hook->arg_list));
            hook->arg_list_size++;
        }

        /** add to pin */
        list_add_tail(&(hook->list), &(pin->hook_list));
    }

    free(line);
    fclose(file);
    close(fd);

    return 0;

    fail:
    errno = errsv;
    return -1;
}

int loadTabs(const char* tabsDir)
{
    int errsv = 0;
    int ret = 0;
    struct dirent* dentry = 0;
    /** for each file in dir */
    int dirfd = open(tabsDir, O_DIRECTORY);
    errsv = errno;

    if(dirfd == -1) {
        syslog(LOG_CRIT, "Couldn't open tabs directory: %s for reading", tabsDir);
        goto fail;
    }

    DIR* dir = fdopendir(dirfd);

    if (dir == 0)
        goto fail;

    while (1) {
        errno = 0;
        dentry = readdir(dir);
        errsv = errno;

        if(errsv != 0)
            goto fail_close_dir;

        if(dentry == 0)
            break;

        ret = loadTab(dirfd, dentry);
        errsv = errno;
    }

    closedir(dir);

    return 0;

    fail_close_dir:
    closedir(dir);

    fail:
    errno = errsv;
    return -1;
}

int freeHook(const struct gpiod_hook* hook)
{
    struct list_head *tmp = 0;
    struct list_head *ag = 0;
    struct gpiod_hook_args* arg = 0;

    list_for_each_safe(ag, tmp, &(hook->arg_list))
    {
        arg = list_entry(ag, struct gpiod_hook_args, list);
        if(arg->c_arg != 0)
            free(arg->c_arg);
        list_del(&(arg->list));
        free(arg);
    }

    return 0;
}

int freeTabs(const struct gpio_pin* pin)
{
    struct list_head *tmp = 0;
    struct list_head *hk = 0;
    struct gpiod_hook* hook = 0;

    list_for_each_safe(hk, tmp, &(pin->hook_list)) {
        hook = list_entry(hk, struct gpiod_hook, list);
        free(hook->path);
        freeHook(hook);
        list_del(&(hook->list));
        free(hook);
    }
}
