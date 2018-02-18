#define _GNU_SOURCE
#include "gpiod-dispatch.h"

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/time.h>

#include "gpiod.h"
#include "gpiod-server.h"

#include "dispatch/gpiod-dispatch-raw.h"

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

int splice_to(int from_fd, int to_fd, ssize_t num)
{
    return splice(from_fd, 0, to_fd, 0, num, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
}

int splice_to_null(int from_fd, ssize_t num)
{
    return splice_to(from_fd, devnullfd, num);
}
