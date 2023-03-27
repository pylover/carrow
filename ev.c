#include "ev.h"

#include <clog.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>


struct evbag {
    int fd;
    struct circuitA *circuit;
    void *state;
    int index;
};


struct evstate {
    int fd;
    unsigned int openmax;
    unsigned int bagscount;
    struct evbag **bags;
};


void 
evinitA(struct circuitA *c, void *s, struct ev *priv) {
    if (priv->state != NULL ) {
        errorA(c, s, "Already initialized");
        return;
    }
   
    priv->state = malloc(sizeof(struct evstate));
    if (priv->state == NULL) {
        FATAL("Out of Memory");
    }
    
    struct evstate *state = priv->state;
    state->fd = epoll_create1(0);
    if (state->fd < 0) {
        errorA(c, s, "epoll_create1");
        return;
    }

    long openmax = sysconf(_SC_OPEN_MAX);
    if (openmax == -1) {
        errorA(c, s, "sysconf(openmax)");
        return;
    }
    state->openmax = openmax;
    state->bagscount = 0;
    state->bags = calloc(openmax, sizeof(struct evbag*));
    returnA(c, s);
}


static void
_bags_freeall(struct evstate *state) {
    int i;
    struct evbag *bag;

    for (i = 0; i < state->openmax; i++) {
        bag = state->bags[i];

        if (bag == NULL) {
            continue;
        }

        free(bag);
        state->bags[i] = NULL;
    }
}


static void 
evclose(struct evstate *state) {
    close(state->fd);
    _bags_freeall(state);
    free(state->bags);
    free(state);
}


void 
evcloseA(struct circuitA *c, void *s, struct ev *priv) {
    evclose(priv->state);
}
