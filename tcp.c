#include "tcp.h"
#include "addr.h"
#include "core.h"

#include <clog.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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


