#include "tcp.h"
#include "addr.h"
#include "core.h"

#include <clog.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>


struct elementA *
listenA(struct circuitA *c, struct tcpsrvstate *s) {
    int fd;
    int option = 1;
    int res;
    struct sockaddr *addr = &(s->bind);
    
    /* Parse listen address */
    carrow_sockaddr_parse(addr, s->bindaddr, s->bindport);
    
    /* Create socket */
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    /* Allow reuse the address */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Bind to tcp port */
    res = bind(fd, addr, sizeof(s->bind)); 
    if (res) {
        return errorA(c, s, "Cannot bind on: %s", carrow_sockaddr_dump(addr));
    }
    
    /* Listen */
    res = listen(fd, s->backlog); 
    INFO("Listening on: %s", carrow_sockaddr_dump(addr));
    if (res) {
        return errorA(c, s, "Cannot listen on: %s", 
                carrow_sockaddr_dump(addr));
    }
    s->listenfd = fd;
    return nextA(c, s);
}


struct elementA *
acceptA(struct circuitA *c, struct tcpsrvstate *s) {
    int fd;
    s->addrlen = sizeof(struct sockaddr);

    fd = accept4(s->listenfd, &(s->newconnaddr), &(s->addrlen), 
            SOCK_NONBLOCK);
    if (fd == -1) {
        if (EVMUSTWAIT()) {
            errno = 0;
            return evwaitA(c, (struct evstate*)s, s->listenfd, EVIN | EVET);
        }
        return errorA(c, s, "accept4");
    }
    
    s->newconnfd = fd;
    return nextA(c, s);
}


struct elementA *
_connect_continue(struct circuitA *c, struct tcpclientstate *s) {
    /* After epoll(2) indicates writability, use getsockopt(2) to read the
       SO_ERROR option at level SOL_SOCKET to determine whether connect()
       completed successfully (SO_ERROR is zero) or unsuccessfully
       (SO_ERROR is one of the usual error codes listed here,
       explaining the reason for the failure).
    */
    int err;
    int l = 4;
    if (s->status == TCSCONNECTING) {
        if (getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &err, &l)) {
            s->status = TCSFAILED;
            return errorA(c, s, "getsockopt");
        }
        if (err != OK) {
            s->status = TCSFAILED;
            errno = err;
            close(s->fd);
            evdearm(s->fd);
            return errorA(c, s, "TCP connect");
        }
        /* Hooray, Connected! */
        s->status = TCSCONNECTED;
    }

    /* Already, Connected */
    socklen_t socksize = sizeof(s->localaddr);
    if (getsockname(s->fd, &(s->localaddr), &socksize)) {
        return errorA(c, s, "getsockname");
    }

    return nextA(c, s);
}


struct elementA *
connectA(struct circuitA *c, struct tcpclientstate *s) {
    if (s->status == TCSCONNECTING) {
        return _connect_continue(c, s);
    }
    int fd;
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *try;

    /* Resolve hostname */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;         /* Allow IPv4 only.*/
    // TODO: IPv6
    // hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    if (getaddrinfo(s->host, s->port, &hints, &result) != 0) {
        return errorA(c, s, "Name resolution failed.");
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address.
    */
    for (try = result; try != NULL; try = try->ai_next) {
        fd = socket(
                try->ai_family,
                try->ai_socktype | SOCK_NONBLOCK,
                try->ai_protocol
            );
        if (fd < 0) {
            continue;
        }

        if (connect(fd, try->ai_addr, try->ai_addrlen) == OK) {
            /* Connection success */
            break;
        }

        if (errno == EINPROGRESS) {
            /* Waiting to connect */
            break;
        }

        close(fd);
        fd = -1;
    }

    if (fd < 0) {
        freeaddrinfo(result);
        return errorA(c, s, "TCP connect");
    }

    /* Connection success */
    /* Update state */
    memcpy(&(s->remoteaddr), try->ai_addr, sizeof(struct sockaddr));
    
    s->fd = fd;
    freeaddrinfo(result);

    if (errno == EINPROGRESS) {
        /* Waiting to connect */
        /* The socket is nonblocking and the connection cannot be
           completed immediately. It is possible to epoll(2) for
           completion by selecting the socket for writing.
        */
        errno = 0;
        s->status = TCSCONNECTING;
        return evwaitA(c, (struct evstate*)s, s->fd, EVOUT);
    }

    /* Seems everything is ok. */
    errno = 0;
    s->status = TCSCONNECTED;
    return _connect_continue(c, s);
}
