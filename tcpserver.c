#include "carrow.h"
#include "addr.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


struct tcpsrvstate {
    struct evstate;

    const char *bindaddr;
    unsigned short bindport;
    int backlog;

    struct sockaddr bind;
    int listenfd;
};


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
    // INFO("binding on: %s", meloop_sockaddr_dump(addr));
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
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr addr;

    // fd = accept4(s->listenfd, &addr, &addrlen, SOCK_NONBLOCK);
    // if (fd == -1) {
    //     if (EVMUSTWAIT()) {
    //         return evwaitA(c, s, s->listenfd, EVIN);
    //     }
    //     else {
    //         return errorA(c, s, "accept4");
    //     }
    //     return;
    // }

    // if (priv->client_connected != NULL) {
    //     priv->client_connected(c, s, fd, &addr);
    // }
    return nextA(c, s);
}


int
main() {
    struct evpriv evpriv = {
        .flags = 0,
    };
    struct tcpsrvstate state = {
        .flags = 0,

        .bindaddr = "0.0.0.0",
        .bindport = 3030,
        .backlog = 2,
    };
    struct circuitA *c = NEW_A(NULL);
                         APPEND_A(c, evinitA, &evpriv);
                         APPEND_A(c, listenA, NULL);
    struct elementA *e = APPEND_A(c, acceptA, NULL);
               loopA(e); 

    runA(c, &state);
    freeA(c);
}
