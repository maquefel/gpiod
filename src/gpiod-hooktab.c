#include "gpiod-hooktab.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>

#include "gpiod-pin.h"
#include "gpiod.h"

const char* gpiod_hook_modifies[] = {
    "EDGE_RISING",
    "EDGE_FALLING",
    "EDGE_BOTH",
    "NO_LOOP",
    "ONESHOT",
    NULL
};

const char* gpiod_special_args_array[HOOKTAB_ARG_MAX] = {
    "$@",
    "$%",
    "$&"
};

enum GPIOD_HOOKTAB_MOD tab_parse_mod(const char* value)
{
    int i = 0;

    do {
        if(strncmp(value, gpiod_hook_modifies[i], strlen(value)) == 0)
            return (enum GPIOD_HOOKTAB_MOD)(i + 1);
    } while(gpiod_hook_modifies[++i] != NULL);

    return HOOKTAB_MOD_MAX;
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
    char* tmp = (char*)str;

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
    char* tmp = (char*)str;

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

    /** open file */
    int fd = openat(dirfd, dentry->d_name, O_RDONLY);
    if(fd == -1) {
        errsv = errno;
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
        uint16_t flags = 0;

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

        /** get modifiers */
        char* coma = 0;
        do {
            tmp1 = eat_space(tmp2);
            tmp2 = next_space(tmp1);

            /** check if coma exists in between */
            len = tmp2 - tmp1;
            coma = memchr((void*)tmp1, 0x2c, len);

            if(coma != NULL) {
                tmp2 = coma;
            }

            len = tmp2 - tmp1;
            tmp = strncpy(buffer, tmp1, len);
            tmp[len] = '\0';

            enum GPIOD_HOOKTAB_MOD mod = tab_parse_mod(tmp);

            if(mod == HOOKTAB_MOD_MAX)
            {
                syslog(LOG_ERR, "line %d : no known event %s", line_num, tmp);
                continue;
            }

            debug_printf_n("modifier : %s", tmp);

            flags |= (uint16_t)mod;

            /** check if coma is next symbol */
            {
                char* tmp3 = eat_space(tmp2);
                if(*tmp3 == 0x02c)
                    coma = tmp2 = tmp3;
            }

            tmp2++;
        } while(coma != 0);

        /** get path */
        tmp1 = eat_space(tmp2);
        tmp2 = next_space(tmp1);
        len = tmp2 - tmp1;
        tmp = strncpy(buffer, tmp1, len);
        tmp[len] = '\0';

        /** allocate a hook here */
        struct gpiod_hook* hook = malloc(sizeof(struct gpiod_hook));
        hook->path = strdup(tmp);
        hook->flags = flags;

        hook->fired = 0;
        hook->spawned = -1;
        hook->arg_list_size = 0;

        INIT_LIST_HEAD(&(hook->list));
        INIT_LIST_HEAD(&(hook->arg_list));


        debug_printf_n("path : %s", hook->path);

        /** get all args */
/* TODO: other flags like timestamp */
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
            debug_printf_n("arg[%d] : %s", i++, tmp);

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
    struct dirent* dentry = 0;
    /** for each file in dir */
    int dirfd = open(tabsDir, O_DIRECTORY);
    if(dirfd == -1) {
        errsv = errno;
        syslog(LOG_CRIT, "Couldn't open tabs directory: %s for reading", tabsDir);
        goto fail;
    }

    DIR* dir = fdopendir(dirfd);
    if (dir == 0) {
        errsv = errno;
        goto fail;
    }

    while (1) {
        errno = 0;
        dentry = readdir(dir);
        errsv = errno;

        if(errsv != 0)
            goto fail_close_dir;

        if(dentry == 0)
            break;

        loadTab(dirfd, dentry);
    }

    closedir(dir);

    return 0;

    fail_close_dir:
    closedir(dir);

    fail:
    errno = errsv;
    return -1;
}

void freeHook(const struct gpiod_hook* hook)
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
}

void freeTabs(const struct gpio_pin* pin)
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
