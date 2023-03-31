#ifndef CARROW_TCP_H
#define CARROW_TCP_H


#include "ev.h"

#include <sys/types.h>
#include <sys/socket.h>


struct tcpsrvstate {
    struct evstate;

    const char *bindaddr;
    unsigned short bindport;
    int backlog;

    struct sockaddr bind;
    int listenfd;

    struct sockaddr newconnaddr;
};


struct elementA *
listenA(struct circuitA *c, struct tcpsrvstate *s);


struct elementA *
acceptA(struct circuitA *c, struct tcpsrvstate *s);


#endif
