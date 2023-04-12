#ifndef CARROW_TCP_H
#define CARROW_TCP_H


#include "ev.h"



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


struct elementA *
listenA(struct circuitA *c, struct tcpsrvstate *s);


struct elementA *
acceptA(struct circuitA *c, struct tcpsrvstate *s);


struct elementA *
connectA(struct circuitA *c, struct tcpclientstate *s);


#endif
