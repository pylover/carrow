#include "ev.h"

#include <clog.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>


struct evbag {
    int fd;
    struct circuitA *circuit;
    struct elementA *current;
    void *state;
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


void 
evcloseA(struct circuitA *c, void *s, struct ev *priv) {
    // TODO: evclose(priv->state);
}


static struct evbag *
_bag_new(struct circuitA *c, void *state, int fd) {
    struct evbag *bag = malloc(sizeof(struct evbag));
    if (bag == NULL) {
        FATAL("Out of memory");
    }
    
    bag->fd = fd;
    bag->circuit = c;
    bag->current = currentA(c);
    bag->state = state;

    return bag;
}


static int
_evdearm(struct evbag *bag) {
    // TODO:
    return 0;
}


static int
_evarm(struct evbag *bag, int flags) {
    // TODO:
    return 0;
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

        _evdearm(bag);
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


struct elementA * 
waitA(struct circuitA *c, void *s, int fd, int flags) {
    struct evbag *bag = _bag_new(c, s, fd);
    
    if (_evarm(bag, flags)) {
        return errorA(c, s, "meloop_ev_arm");
    }

    /* Returning null to stop executing the next task */
    return NULL;
}


/** event loop 
*/
int
evloop(volatile int *status, struct evstate *state) {
    struct epoll_event events[state->openmax];
    struct epoll_event ev;
    struct evbag *bag;
    int i;
    int nfds;
    int fd;
    int ret = OK;

    while (((status == NULL) || (*status > EXIT_FAILURE)) &&
            (state->bagscount)) {
        nfds = epoll_wait(state->fd, events, state->openmax, -1);
        if (nfds < 0) {
            ret = ERR;
            break;
        }

        if (nfds == 0) {
            ret = OK;
            break;
        }

        for (i = 0; i < nfds; i++) {
            ev = events[i];
            bag = (struct evbag *) ev.data.ptr;
            fd = bag->fd;
            
            continueA(bag->circuit, bag->current, bag->state);;
        }
    }

    return ret;
}
