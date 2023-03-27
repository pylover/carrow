#ifndef CARROW_EV_H
#define CARROW_EV_H


#include "core.h"


struct evstate;


struct ev {
    int flags;
    struct evstate *state;
};


void 
evinitA(struct circuitA *c, void *s, struct ev *priv);


void 
evcloseA(struct circuitA *c, void *s, struct ev *priv);


#endif
