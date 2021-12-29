#include "gpiod-event-loop.h"

#include "gpiod-pin.h"
#include "gpiod-server.h"
#include "gpiod-dispatch.h"

#include "list.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>

static int shutdown_flag = 0;
static int hup_flag = 0;

static LIST_HEAD(p_list); /** poll list */
static LIST_HEAD(i_list); /** interrupt list */
static LIST_HEAD(e_list); /** wrapper list for cleanup */

LIST_HEAD(tcp_clients);

/** wrappers */
static struct epoll_wrapper signalfd_w;
static struct epoll_wrapper listenfd_w;
static struct epoll_wrapper timerfd_w;

/** signalfd */
static int listenfd;

/** timerfd */
static int timerfd;
int poll_period = 50;

int add_to_epoll(int epollfd, struct gpio_pin* pin)
{
    int errsv = 0;
    int fd = 0;
    struct epoll_wrapper* ew = malloc(sizeof(struct epoll_wrapper));

    switch(pin->facility) {
        case GPIOD_FACILITY_SYSFS:
            ew->event.events = EPOLLPRI | EPOLLERR;
            ew->type = SYSFS_PIN;
            break;
        case GPIOD_FACILITY_UAPI:
            ew->event.events = EPOLLIN;
            ew->type = UAPI_PIN;
            break;
        default:
            errsv = EINVAL;
            goto free_ew;
    }

    fd = pin->fd;

    ew->pin = pin;

    ew->event.data.ptr = ew;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ew->event) == -1) {
        errsv = errno;
        syslog(LOG_CRIT, "add_to_epoll : epoll_ctl failed with [%d] : %s", errsv, strerror(errsv));
        goto free_ew;
    }

    list_add_tail(&(ew->list), &(e_list));

    return 0;

    free_ew:
    free(ew);
    errno = errsv;
    return -1;
}

int loop(int sigfd)
{
    int ret = 0;
    int epollfd = 0;
    int errsv = 0;
    int epoll_events = 0;

    epollfd = epoll_create1(EPOLL_CLOEXEC);

    struct list_head* pos = 0;
    struct list_head* tmp = 0;
    struct gpio_pin* pin = 0;

    // list_move_tail
    list_for_each_safe(pos, tmp, &gp_list) {
        pin = list_entry(pos, struct gpio_pin, list);
        switch(pin->edge) {
            case GPIOD_EDGE_BOTH:
            case GPIOD_EDGE_FALLING:
            case GPIOD_EDGE_RISING:
                list_move_tail(&(pin->list), &(i_list));
                /** add fd to epoll */
                break;
            case GPIOD_EDGE_POLLED:
            default:
                list_move_tail(&(pin->list), &(p_list));
                break;
        }
    }

    /** initialize signalfd */
    struct epoll_event signalfd_event = {0};
    signalfd_w.type = SIGNAL_FD;
    signalfd_w.fd = sigfd;

    signalfd_event.events = EPOLLIN | EPOLLET;
    signalfd_event.data.ptr = &signalfd_w;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sigfd, &signalfd_event) == -1) {
        errsv = errno;
        syslog(LOG_CRIT, "epoll_ctl : signalfd failed with [%d] : %s", errsv, strerror(errsv));
        goto splice_lists;
    }

    epoll_events++;

    /** establish listening port */
    /** initialize listening socket */
    listenfd = init_socket(addr, port);
    errsv = errno;

    if(listenfd < 0) {
        syslog(LOG_CRIT, "error initializing listening socket on port : %d", port);
        goto splice_lists;
    }

    /** make listen non-block */
    make_non_blocking(listenfd);
    errsv = errno;

    if (ret != 0) {
        syslog(LOG_CRIT, "make_non_blocking for listen port failed with [%d] : %s", ret, strerror(ret));
        goto splice_lists;
    }

    /** make listen */
    ret = listen (listenfd, 1);
    errsv = errno;

    if(ret == -1) {
        syslog(LOG_CRIT, "listen failed with [%d] : %s", errsv, strerror(errsv));
        goto close_socket;
    }

    /** add to epoll watch */
    listenfd_w.fd = listenfd;
    listenfd_w.type = LISTEN_PORT;
    listenfd_w.event.events = EPOLLIN;
    listenfd_w.event.data.ptr = &listenfd_w;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd_w.fd, &listenfd_w.event) == -1) {
        errsv = errno;
        syslog(LOG_CRIT, "epoll_ctl : listen_port failed with [%d] : %s", errsv, strerror(errsv));
        goto close_socket;
    }

    epoll_events++;

    /** setup timerfd for polled gpios */
    /** @note we can specify dirrent polling periods for different pins */
    //  struct timespec {
    //      time_t tv_sec;                /* Seconds */
    //      long   tv_nsec;               /* Nanoseconds */
    //  };
    //
    //  struct itimerspec {
    //      struct timespec it_interval;  /* Interval for periodic timer */
    //      struct timespec it_value;     /* Initial expiration */
    //  };
    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if(timerfd == -1) {
        syslog(LOG_CRIT, "timerfd_settime failed with [%d] : %s", errsv, strerror(errsv));
        goto close_socket;
    }

    struct itimerspec ts = {
        {0, poll_period * 1000000UL},
        {0, poll_period * 1000000UL}
    };

    ret = timerfd_settime(timerfd, 0, &ts, 0);
    if(ret == -1) {
        syslog(LOG_CRIT, "timerfd_settime failed with [%d] : %s", errsv, strerror(errsv));
        goto close_timerfd;
    }

    timerfd_w.fd = timerfd;
    timerfd_w.type = TIMER_FD;
    timerfd_w.event.events = EPOLLIN;
    timerfd_w.event.data.ptr = &timerfd_w;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, timerfd_w.fd, &timerfd_w.event) == -1) {
        errsv = errno;
        syslog(LOG_CRIT, "epoll_ctl : adding timerfd failed with [%d] : %s", errsv, strerror(errsv));
        goto close_socket;
    }

    epoll_events++;

    /** add interupt pins */
    list_for_each(pos, &i_list) {
        pin = list_entry(pos, struct gpio_pin, list);
        ret = add_to_epoll(epollfd, pin);

        if(ret == 0)
            epoll_events++;
    }

    struct epoll_event* events = malloc(sizeof(struct epoll_event)*epoll_events);

    while(!shutdown_flag) {
        struct timeval t1 = {0}; // [sec], [us]
        struct timeval t2 = {0};
        struct timespec ts = {0};

        enum GPIOD_EDGE event = GPIOD_EDGE_MAX;

        gettimeofday(&t1, NULL);
        int nfds = epoll_wait(epollfd, events, epoll_events, -1);
        gettimeofday(&t2, NULL);

        struct epoll_wrapper* w = 0;

        for(int i = 0; i < nfds; i++) {
            w = (struct epoll_wrapper*)(events[i].data.ptr);

            switch(w->type) {
                case LISTEN_PORT:
                {
                    struct tcp_client* client_ = accept_tcp_client(w->fd);
                    syslog(LOG_DEBUG, "LISTEN_PORT client connected");

                    /** overriding wrapper */
                    w = &client_->ewrap;

                    w->type = CLIENT_PORT;
                    w->client = client_;

                    w->event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                    w->event.data.ptr = w;

                    list_add_tail(&(client_->list), &tcp_clients);

                    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, client_->fd, &w->event) != 0) {
                        errsv = errno;
                        syslog(LOG_CRIT, "LISTEN_PORT epoll_ctl failed with [%d] : %s", errsv, strerror(errsv));
                        break;
                    }
                }
                break;
                case CLIENT_PORT: {
                    struct tcp_client* client_ = w->client;
                    int len;
                    ioctl(client_->fd, FIONREAD, &len);
                    if(!len && (events[i].events & EPOLLRDHUP)) {
                        /** @todo add remote ip and port info */
                        syslog(LOG_DEBUG, "client disconnected");
                        delete_tcp_client(client_);
                    }  else
                        splice_to_null(client_->fd, len);
                } break;
                case SYSFS_PIN:
                case UAPI_PIN:
                    /** TODO: add debounce check */
                    pin = w->pin;
                    if(!pin->changed(pin, &ts, (int8_t*)&event) && pin->edge == GPIOD_EDGE_BOTH) {
                        syslog(LOG_DEBUG, "spurious interrupt on %s:%d", pin->label, pin->system);
                        continue;
                    }

                    syslog(LOG_DEBUG, "PIN event fired %s:%d = %d", pin->label, pin->system, pin->value_);

                    ret = dispatch(ts, pin->local, pin->value_);
                    ret = dispatch_hooks(pin, event);
                break;
                case TIMER_FD: {
                    /** readout the timer */
                    uint64_t exp;
                    while(read(timerfd, &exp, sizeof(uint64_t)) > 0)
                        ;
                    list_for_each(pos, &p_list) {
                        pin = list_entry(pos, struct gpio_pin, list);
                        int8_t changed = pin->changed(pin, &ts, (int8_t*)&event);
                        if(changed) {
                            debug_printf_n("pin[%d] value changed : %u", pin->system, pin->value_);
                            ret = dispatch(ts, pin->local, pin->value_);
                            ret = dispatch_hooks(pin, event);
                        }
                    }
                } break;
                case SIGNAL_FD: {
                    syslog(LOG_DEBUG, "SIGNAL_FD event fired");
                    struct signalfd_siginfo fdsi = {0};
                    ssize_t len;
                    len = read(sigfd, &fdsi, sizeof(struct signalfd_siginfo));

                    if (len != sizeof(struct signalfd_siginfo))
                        syslog(LOG_CRIT, "reading sigfd failed");

                    switch(fdsi.ssi_signo)
                    {
                        case SIGINT:
                        case SIGTERM:
                            syslog(LOG_DEBUG, "SIGTERM or SIGINT signal recieved - shutting down...");
                            shutdown_flag = 1;
                            break;
                        case SIGHUP:
                            hup_flag = 1;
                            break;
                        case SIGCHLD:
                            syslog(LOG_INFO, "SIGCHLD signal recieved - child [%d] finished with status %d...", fdsi.ssi_pid, fdsi.ssi_status);
                            pid_t pid = waitpid(fdsi.ssi_pid, 0, WNOHANG);
                            if(pid != -1)
                                hook_clear_spawned(fdsi.ssi_pid);
                            break;
                        default:
                            break;
                    }
                } break;
                default:
                    break;
            }
        }
    }

    /** disconnect clients */
    struct tcp_client* c = 0;
    // list_move_tail
    list_for_each_safe(pos, tmp, &tcp_clients) {
        c = list_entry(pos, struct tcp_client, list);
        delete_tcp_client(c);
    }

    /** free wrappers */
    struct epoll_wrapper* w = 0;
    list_for_each_safe(pos, tmp, &e_list) {
        w = list_entry(pos, struct epoll_wrapper, list);
        free(w);
    }

    free(events);

    close_timerfd:
    close(timerfd);

    close_socket:
    close(listenfd);

    splice_lists:
    list_splice(&i_list, &gp_list);
    list_splice(&p_list, &gp_list);

    /** @todo return codes */
    return 0;
}
