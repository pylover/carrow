#include "ev.h"

#include <clog.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>


struct evbag {
    struct circuitA *circuit;
    struct elementA *current;
    struct evstate *state;
};


struct evglobalstate {
    int fd;
    unsigned int openmax;
    unsigned int bagscount;
    struct evbag **bags;
};


struct elementA *
evinitA(struct circuitA *c, struct evstate *s, struct ev *priv) {
    if (priv->globalstate != NULL ) {
        return errorA(c, s, "Already initialized");
    }
   
    priv->globalstate = malloc(sizeof(struct evglobalstate));
    if (priv->globalstate == NULL) {
        FATAL("Out of Memory");
    }
    
    struct evglobalstate *globalstate = priv->globalstate;
    globalstate->fd = epoll_create1(0);
    if (globalstate->fd < 0) {
        return errorA(c, s, "epoll_create1");
    }

    long openmax = sysconf(_SC_OPEN_MAX);
    if (openmax == -1) {
        return errorA(c, s, "sysconf(openmax)");
    }
    globalstate->openmax = openmax;
    globalstate->bagscount = 0;
    globalstate->bags = calloc(openmax, sizeof(struct evbag*));
    return nextA(c, s);
}


void 
evcloseA(struct circuitA *c, struct evstate *s, struct ev *priv) {
    // TODO: evclose(priv->globalstate);
}


static struct evbag *
_bag_new(struct circuitA *c, struct evstate *state) {
    struct evbag *bag = malloc(sizeof(struct evbag));
    if (bag == NULL) {
        FATAL("Out of memory");
    }
    
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
_evarm(struct evbag *bag) {
    // TODO:
    return 0;
}


static void
_bags_freeall(struct evglobalstate *globalstate) {
    int i;
    struct evbag *bag;

    for (i = 0; i < globalstate->openmax; i++) {
        bag = globalstate->bags[i];

        if (bag == NULL) {
            continue;
        }

        _evdearm(bag);
        free(bag);
        globalstate->bags[i] = NULL;
    }
}


static void 
evclose(struct evglobalstate *globalstate) {
    close(globalstate->fd);
    _bags_freeall(globalstate);
    free(globalstate->bags);
    free(globalstate);
}


struct elementA * 
waitA(struct circuitA *c, struct evstate *s) {
    struct evbag *bag = _bag_new(c, s);
    
    if (_evarm(bag)) {
        return errorA(c, s, "meloop_ev_arm");
    }

    /* Returning null to stop executing the next task */
    return NULL;
}


/** event loop 
*/
int
evloop(volatile int *status, struct evglobalstate *globalstate) {
    struct epoll_event events[globalstate->openmax];
    struct epoll_event ev;
    struct evbag *bag;
    int i;
    int nfds;
    int fd;
    int ret = OK;

    while (((status == NULL) || (*status > EXIT_FAILURE)) &&
            (globalstate->bagscount)) {
        nfds = epoll_wait(globalstate->fd, events, globalstate->openmax, -1);
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
            continueA(bag->circuit, bag->current, bag->state);;
        }
    }

    return ret;
}
