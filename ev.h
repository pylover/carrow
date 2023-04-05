#ifndef CARROW_EV_H
#define CARROW_EV_H


#include "core.h"

#include <sys/epoll.h>
#include <stdbool.h>


#define EVMUSTWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK))
#define EVIN      EPOLLIN
#define EVOUT     EPOLLOUT
#define EVET      EPOLLET
#define EVONESHOT EPOLLONESHOT


struct evglobalstate;


struct evpriv {
    int epollflags;
};


struct evstate {
    int fd;
    int events;
};


struct elementA *
evinitA(struct circuitA *c, struct evstate *s);


void 
evdeinitA();


void 
evcloseA(struct circuitA *c, struct evstate *s, struct evpriv *priv);


struct elementA * 
evwaitA(struct circuitA *c, struct evstate *s, int fd, int op);


int
evloop(volatile int *status);


bool
isfailedA(struct circuitA *c);


int
evdearm(int fd);


#endif
