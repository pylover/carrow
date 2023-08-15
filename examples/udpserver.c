// Copyright 2023 Vahid Mardani
/*
 * This file is part of Carrow.
 *  Carrow is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  Carrow is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Carrow. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 *
 *
 * An edge-triggered(epoll(7)) example using carrow.
 */
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include <mrb.h>
#include <clog.h>

#include "tty.h"
#include "carrow.h"
#include "addr.h"


/* udp server carrow types and function */
typedef struct udpserver {
    const char *bindaddr;
    unsigned short bindport;
    int backlog;
} udpserver;


#undef CARROW_ENTITY
#define CARROW_ENTITY udpserver
#include "carrow_generic.h"  // NOLINT
#include "carrow_generic.c"  // NOLINT


#define BUFFSIZE 1536


static void
listenA(struct udpserver_coro *self, struct udpserver *state) {
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr bindaddr;
    struct sockaddr connaddr;
    static int fd;
    int connfd;
    int res;
    int option = 1;
    CORO_START;

    /* Parse listen address */
    sockaddr_parse(&bindaddr, state->bindaddr, state->bindport);

    /* Create socket */
    fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

    /* Allow reuse the address */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Bind to udp port */
    res = bind(fd, &bindaddr, sizeof(bindaddr));
    if (res) {
        CORO_REJECT("Cannot bind on: %s", sockaddr_dump(&bindaddr));
    }

    CORO_WAITFOR(echoA, state);

    CORO_FINALLY;
    CORO_CLEANUP;
    CORO_END;
}


int
main() {
    clog_verbosity = CLOG_DEBUG;

    if (carrow_handleinterrupts()) {
        return EXIT_FAILURE;
    }

    struct udpserver state = {
        .bindaddr = "0.0.0.0",
        .bindport = 3030,
        .backlog = 2,
    };

    return udpserver_forever(listenA, &state, NULL);
}
