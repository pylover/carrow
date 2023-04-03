#ifndef CARROW_TCP_H
#define CARROW_TCP_H


#include "ev.h"

#include <sys/types.h>
#include <sys/socket.h>


struct tcpsrvstate {
    struct evstate;

    const char *bindaddr;
    signed short bindport;
    int backlog;

    struct sockaddr bind;
    int listenfd;

    struct sockaddr newconnaddr;
    socklen_t addrlen;
    int newconnfd;
};


enum tcpclientstatus {
    TCSIDLE,
    TCSCONNECTING,
    TCSCONNECTED,
    TCSFAILED,
};


struct tcpclientstate {
    struct evstate;

    const char *host;
    const char *port;
    enum tcpclientstatus status;

    struct sockaddr localaddr;
    struct sockaddr remoteaddr;
};


struct elementA *
listenA(struct circuitA *c, struct tcpsrvstate *s);


struct elementA *
acceptA(struct circuitA *c, struct tcpsrvstate *s);


struct elementA *
connectA(struct circuitA *c, struct tcpclientstate *s);


#endif
