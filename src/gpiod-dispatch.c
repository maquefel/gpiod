#define _GNU_SOURCE
#include "gpiod-dispatch.h"

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/time.h>

#include <syslog.h>

#include "gpiod.h"
#include "gpiod-server.h"

#include "dispatch/gpiod-dispatch-raw.h"
#include "gpiod-pin.h"
#include "gpiod-hooktab.h"
#include "gpiod-exec.h"

int dispatch(struct timeval ts, uint32_t chan, uint8_t value)
{
    char * package = 0;
    size_t len = 0;
    int errsv = 0;
    int ret = 0;

    len = dispatch_raw_pack(&package, ts, chan, value);

    struct list_head* pos = 0;
    struct tcp_client* client = 0;

    list_for_each(pos, &tcp_clients) {
        client = list_entry(pos, struct tcp_client, list);
        ret = write(client->fd, package, len);
        errsv = errno;
        if(ret != len)
            goto fail;
    }

    return 0;

    fail:
    errno = errsv;
    return -1;
}

char* test_text_event = "TEST";
char* test_num_event = "111";

int dispatch_hooks(struct gpio_pin* pin)
{
    int errsv = 0;

    if(list_empty(&(pin->hook_list)))
        return 0; /** empty list is a good list*/

    struct gpiod_hook *hook = 0;
    struct list_head *pos = 0;

    list_for_each(pos, &(pin->hook_list)) {
        hook = list_entry(pos, struct gpiod_hook, list);
        /** @todo check last event */
        /** form  args list */
        char *argv[hook->arg_list_size + 1];
        char *envp[] = {0};

        struct list_head* pos_arg = 0;
        struct gpiod_hook_args* arg = 0;

        int i = 0;
        // path to executable as argv[0]
        argv[i++] = hook->path;

        list_for_each(pos_arg, &(hook->arg_list)) {
            arg = list_entry(pos_arg, struct gpiod_hook_args, list);

            if(arg->arg == HOOKTAB_ARG_MAX) {
                argv[i++] = arg->c_arg;
                continue;
            }

            switch(arg->arg) {
                case HOOKTAB_ARG_LABEL:
                    argv[i++] = pin->label;
                    break;
                case HOOKTAB_ARG_EVENT_TEXT:
                    argv[i++] = test_text_event;
                    break;
                case HOOKTAB_ARG_EVENT_NUM:
                    argv[i++] = test_num_event;
                    break;
                default:
                    break;
            }
        }

        // @note will return only on failure
        int ret = exec_start(hook->path, argv, envp);
        errsv = errno;

        // -1 - path not exists
        if(ret == -1) {
            syslog(LOG_ERR, "failed launching hook %s with %d : %s", hook->path, errsv, strerror(errsv));
        }

        // 0 - we have forked and execvpe failed so should exit
        if(ret == 0) {
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

int splice_to(int from_fd, int to_fd, ssize_t num)
{
    return splice(from_fd, 0, to_fd, 0, num, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
}

int splice_to_null(int from_fd, ssize_t num)
{
    return splice_to(from_fd, devnullfd, num);
}
