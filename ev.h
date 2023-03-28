#ifndef CARROW_EV_H
#define CARROW_EV_H


#include "core.h"


struct evglobalstate;


struct ev {
    int flags;
    struct evglobalstate *globalstate;
};


struct evstate {
    int fd;
    int flags;
    int events;
};


struct elementA *
evinitA(struct circuitA *c, struct evstate *s, struct ev *priv);


void 
evcloseA(struct circuitA *c, struct evstate *s, struct ev *priv);


#endif
