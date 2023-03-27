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


struct elementA *
evinitA(struct circuitA *c, void *s, struct ev *priv) {
    if (priv->state != NULL ) {
        return errorA(c, s, "Already initialized");
    }
   
    priv->state = malloc(sizeof(struct evstate));
    if (priv->state == NULL) {
        FATAL("Out of Memory");
    }
    
    struct evstate *state = priv->state;
    state->fd = epoll_create1(0);
    if (state->fd < 0) {
        return errorA(c, s, "epoll_create1");
    }

    long openmax = sysconf(_SC_OPEN_MAX);
    if (openmax == -1) {
        return errorA(c, s, "sysconf(openmax)");
    }
    state->openmax = openmax;
    state->bagscount = 0;
    state->bags = calloc(openmax, sizeof(struct evbag*));
    return nextA(c, s);
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
// 
// 
// void 
// waitA(struct circuitA *c, void *s, int fd, int flags) {
//     struct bagS *bag = meloop_bag_new(c, s, fd);
//     
//     if (meloop_ev_arm(op | flags, bag)) {
//         ERROR_A(c, s, data, "meloop_ev_arm");
//     }
// }
