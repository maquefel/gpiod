#include "gpiod-server.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#include <unistd.h>
#include <fcntl.h>

int port;
uint32_t addr;

int init_socket(uint32_t ipaddr, int portno)
{
    struct sockaddr_in addr;
    int ret = 0;
    int errsv = 0;

    /* First call to socket() function */
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    errsv = errno;
    if (ret == -1)
    {
        switch (errsv)
        {
            case EPROTONOSUPPORT:
                break;
            case EMFILE:
                break;
            case ENFILE:
                break;
            case EACCES:
                break;
            case ENOBUFS:
                break;
            case EINVAL:
                break;
            default:
                break;
        }
        syslog(LOG_CRIT, "socket open failure with [%d] : %s", errsv, strerror(errsv));
        goto fail;
    }

#ifdef DEBUG
    int on = 1;
    ret = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
    errsv = errno;

    if(ret == -1) {
        syslog(LOG_CRIT, "setsockopt SO_REUSEADDR failure with [%d] : %s", errsv, strerror(errsv));
    }
#endif

    /* Initialize socket structure */
    memset((char *) &addr, 0, sizeof(addr));

    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = ipaddr;
    addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        errsv = errno;
        syslog(LOG_CRIT, "ERROR on binding %s:%d", inet_ntoa(addr.sin_addr), portno);
        goto fail;
    }

    return sockfd;

    fail:
    errno = errsv;
    return -1;
}

struct tcp_client* accept_tcp_client(int fd)
{
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    clilen = sizeof(cli_addr);
    int errsv = 0;

    struct tcp_client* client = 0;

    newsockfd = accept(fd, (struct sockaddr *) &cli_addr, &clilen);
    errsv = errno;
    // EAGAIN EWOULDBLOCK
    if (newsockfd < 0)
    {
        errno = errsv;
        goto quit;
    }

    int optval = 1;
    socklen_t optlen = sizeof(optval);

    if(setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &optval, optlen) < 0) {
        errsv = errno;
        close(newsockfd);
        goto quit;
    }

    syslog(LOG_INFO, "Client connected: %s:%d", inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
    client = malloc(sizeof(struct tcp_client));

    client->fd = newsockfd;
    client->ip = cli_addr.sin_addr;
    client->port = cli_addr.sin_port;

    INIT_LIST_HEAD(&(client->list));

    quit:
    errno = errsv;
    return client;
}

void delete_tcp_client(struct tcp_client* client)
{
    shutdown(client->fd, SHUT_RDWR);
    close(client->fd);
    /** free tcp client */
    list_del(&(client->list));
    free(client);
}

int make_non_blocking(int fd)
{
    int flags, s;
    int errsv = 0;

    flags = fcntl (fd, F_GETFL, 0);
    errsv = errno;

    if (flags == -1)
        goto fail;

    flags |= O_NONBLOCK;
    s = fcntl (fd, F_SETFL, flags);
    errsv = errno;

    if (s == -1)
        goto fail;

    return 0;

    fail:
    errno = errsv;
    return -1;
}
