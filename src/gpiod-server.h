#ifndef __GPIOD_SERVER_H__
#define __GPIOD_SERVER_H__

#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdint.h>

#include "list.h"

extern int port;
extern uint32_t addr;
extern struct list_head tcp_clients;

// SYFS PIN
// UAPI PIN
// TCP CLIENT
// LISTEN
// ACCEPT
// PROTOSERVER

/// loop types used in single epoll/pselect loop
enum loop_type {
    SYSFS_PIN,          ///< event from uapi gpio pin
    UAPI_PIN,           ///< event from sysfs gpio pin
    LISTEN_PORT,        ///< port for incoming tcp connections
    CLIENT_PORT,        ///< port for incoming data from logged in tcp clients
    REMOTE_PORT,        ///< port for data from outgoing connections
    SIGNAL_FD,          ///< signals watch file descriptor
    TIMER_FD,           ///< timer file descriptor
    LOOP_TYPE_MAX
};

struct epoll_wrapper {
    enum loop_type type; ///> type of wrapper
    struct epoll_event event; ///> event for passing to epoll_ctl
    struct list_head list;
    union {
        int fd; ///> file descriptor if type is SIGNAL_FD | SYSFS_PIN | LISTEN_PORT
        struct gpio_pin* pin; ///> wrapper contains this pointer if type is UAPI_PIN
        struct tcp_client* client; ///> if type is CLIENT_PORT
    };
};

struct tcp_client {
    int fd; ///> external connection fd

    /** client stats */
    struct in_addr ip; ///> address of connected client, 0 - if no one is connected
    in_port_t port; ///> port of connected client, 0 - if no one is connected

    struct epoll_wrapper ewrap;

    struct list_head list;
};

int init_socket(uint32_t /*ipaddr*/, int /*portno*/);
struct tcp_client* accept_tcp_client(int /*fd*/);
void delete_tcp_client(struct tcp_client* /*client*/);
int make_non_blocking(int /*fd*/);

#endif
