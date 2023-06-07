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
 */
#include "tty.h"
#include "carrow.h"
#include "addr.h"

#include <clog.h>
#include <mrb.h>

#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>


typedef struct tcpconn {
    const char *hostname;
    const char *port;

    int fd;
    struct sockaddr localaddr;
    struct sockaddr remoteaddr;
    mrb_t inbuff;
    mrb_t outbuff;
} tcpconn;


#undef CARROW_ENTITY
#define CARROW_ENTITY tcpconn
#include "carrow_generic.h"
#include "carrow_generic.c"


#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


static void
ioA(struct tcpconn_coro *self, struct tcpconn *conn) {
    ssize_t bytes = 0;
    struct mrb *in = conn->inbuff;
    struct mrb *out = conn->outbuff;
    CORO_START;

    while (true) {
        /* stdin read */
        if (mrb_isfull(out)) {
            CORO_REJECT("outbuff full");
        }

        CORO_WAIT(STDIN_FILENO, CIN);
        bytes = mrb_readin(out, STDIN_FILENO, mrb_available(out));
        if (bytes <= 0) {
            CORO_REJECT("read(stdin)");
        }

        /* tcp write */
        CORO_WAIT(conn->fd, COUT);
        bytes = mrb_writeout(out, conn->fd, mrb_used(out));
        if (bytes <= 0) {
            CORO_REJECT("write(tcp:%d)", conn->fd);
        }

        /* tcp read */
        if (mrb_isfull(in)) {
            CORO_REJECT("inbuff full");
        }

        CORO_WAIT(conn->fd, CIN);
        bytes = mrb_readin(in, conn->fd, mrb_available(in));
        if (bytes <= 0) {
            CORO_REJECT("read(tcp:%d)", conn->fd);
        }

        /* stdout write */
        CORO_WAIT(STDOUT_FILENO, COUT);
        bytes = mrb_writeout(in, STDOUT_FILENO, mrb_used(in));
        if (bytes <= 0) {
            CORO_REJECT("write(stdout)");
        }
    }

    CORO_FINALLY;
    if (bytes == 0) {
        ERROR("EOF");
    }
    tcpconn_evloop_unregister(STDIN_FILENO);
    tcpconn_evloop_unregister(STDOUT_FILENO);
    if (conn->fd != -1) {
        tcpconn_evloop_unregister(conn->fd);
        close(conn->fd);
    }

    errno = 0;
    CORO_END;
}


static void
connectA(struct tcpconn_coro *self, struct tcpconn *conn) {
    int err = 0;
    int optlen = 4;
    int ret;
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *try;
    int cfd;
    CORO_START;

    /* Resolve hostname */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;         /* Allow IPv4 only.*/
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    if (getaddrinfo(conn->hostname, conn->port, &hints, &result) != 0) {
        CORO_REJECT("getaddrinfo");
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address.
    */
    for (try = result; try != NULL; try = try->ai_next) {
        cfd = socket(try->ai_family, try->ai_socktype | SOCK_NONBLOCK,
                try->ai_protocol);
        if (cfd < 0) {
            continue;
        }

        if (connect(cfd, try->ai_addr, try->ai_addrlen) == 0) {
            /* Connection success */
            break;
        }

        if (errno == EINPROGRESS) {
            /* Waiting to connect */
            break;
        }

        close(cfd);
        cfd = -1;
    }

    if (cfd < 0) {
        freeaddrinfo(result);
        CORO_REJECT("connect");
    }

    /* Address resolved, updating conn. */
    memcpy(&(conn->remoteaddr), try->ai_addr, sizeof(struct sockaddr));
    conn->fd = cfd;
    freeaddrinfo(result);
    if (errno == EINPROGRESS) {
        /* Connection is in-progress, Waiting. */
        /* The socket is nonblocking and the connection cannot be
           completed immediately. It is possible to epoll(2) for
           completion by selecting the socket for writing.
        */
        CORO_WAIT(conn->fd, COUT);

        /* After epoll(2) indicates writability, use getsockopt(2) to read the
           SO_ERROR option at level SOL_SOCKET to determine whether connect()
           completed successfully (SO_ERROR is zero) or unsuccessfully
           (SO_ERROR is one of the usual error codes listed here,
           explaining the reason for the failure).
        */
        ret = getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &err, &optlen);
        if (ret || err) {
            errno = err;
            CORO_REJECT("getsockopt");
        }
    }

    /* Hooray, Connected! */
    errno = 0;
    socklen_t socksize = sizeof(conn->localaddr);
    if (getsockname(conn->fd, &(conn->localaddr), &socksize)) {
        CORO_REJECT("getsockname");
    }
    INFON("Connected: %s", sockaddr_dump(&conn->localaddr));
    INFOH(" -> %s", sockaddr_dump(&conn->remoteaddr));

    CORO_FINALLY;
    if (errno != 0) {
        if (conn->fd != -1) {
            tcpconn_evloop_unregister(conn->fd);
            close(conn->fd);
        }

        errno = 0;
        CORO_END;
    }
    CORO_NEXT(ioA);
}


int
main() {
    int ret = EXIT_SUCCESS;
    clog_verbosity = CLOG_DEBUG;

    if (carrow_handleinterrupts()) {
        return EXIT_FAILURE;
    }

    /* Non blocking starndard input/output */
    if (stdin_nonblock() || stdout_nonblock()) {
        return EXIT_FAILURE;
    }

    struct tcpconn conn = {
        .hostname = "localhost",
        .port = "3030",
    };

    conn.inbuff = mrb_create(BUFFSIZE);
    conn.outbuff = mrb_create(BUFFSIZE);
    if (conn.inbuff == NULL || conn.outbuff == NULL) {
        ERROR("Cannot initialized buffers.");
        return EXIT_FAILURE;
    }

    ret = tcpconn_forever(connectA, &conn, NULL);
    if (mrb_destroy(conn.inbuff) || mrb_destroy(conn.outbuff)) {
        ERROR("Cannot dispose buffers.");
        ret = EXIT_FAILURE;
    }
    return ret;
}
