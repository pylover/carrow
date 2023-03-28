#ifndef CARROW_EV_H
#define CARROW_EV_H


#include "core.h"

#include <sys/epoll.h>


#define EVMUSTWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK))
#define EVIN    EPOLLIN
#define EVOUT   EPOLLOUT


struct evglobalstate;


struct evpriv {
    int flags;
    struct evglobalstate *globalstate;
};


struct evstate {
    int fd;
    int flags;
    int events;
};


struct elementA *
evinitA(struct circuitA *c, struct evstate *s, struct evpriv *priv);


void 
evcloseA(struct circuitA *c, struct evstate *s, struct evpriv *priv);


struct elementA * 
evwaitA(struct circuitA *c, struct evstate *s, int fd, int op);


#endif
