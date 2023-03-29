#include "evbag.h"
#include "ev.h"

#include <clog.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>


static int epollfd = -1;


struct elementA *
evinitA(struct circuitA *c, struct evstate *s, struct evpriv *priv) {
    if (epollfd != -1 ) {
        return errorA(c, s, "Already initialized");
    }
  
    if (evbag_init()) {
        return errorA(c, s, "Cannot initialize event bag");
    }

    epollfd = epoll_create1(0);
    if (epollfd < 0) {
        return errorA(c, s, "epoll_create1");
    }

    return nextA(c, s);
}


static void
bags_freeall() {
}



static int
_evdearm(struct evbag *bag) {
    // TODO:
    return 0;
}


static void 
_evclose() {
    int i;
    struct evbag *bag;

    close(epollfd);
    epollfd = -1;

    for (i = 0; i < evbag_max(); i++) {
        bag = evbag_get(i);

        if (bag == NULL) {
            continue;
        }

        _evdearm(bag);
        evbag_free(bag->state->fd);
    }
}


void 
evdeinitA(struct circuitA *c, struct evstate *s, struct evpriv *priv) {
    _evclose();
}


static int
_evarm(struct evbag *bag) {
    // struct epoll_event ev;
    // 
    // ev.events = bag->flags;
    // ev.data.ptr = bag;
    //  
    // if (epoll_ctl(_epfd, EPOLL_CTL_MOD, bag->fd, &ev) != OK) {
    //     if (errno == ENOENT) {
    //         errno = 0;
    //         /* File descriptor is not exists yet */
    //         if (epoll_ctl(_epfd, EPOLL_CTL_ADD, bag->fd, &ev) != OK) {
    //             return ERR;
    //         }
    //     }
    //     else {
    //         return ERR;
    //     }
    // }
    // 
    // return OK;

    // TODO:
    return 0;
}


struct elementA * 
evwaitA(struct circuitA *c, struct evstate *s, int fd, int op) {
    s->fd = fd;
    s->flags = op;
    struct evbag *bag = bag_new(c, s);
    
    if (_evarm(bag)) {
        return errorA(c, s, "_evarm");
    }

    /* Returning null to stop executing the next task */
    return NULL;
}


/** event loop 
*/
int
evloop(volatile int *status) {
    unsigned int evmax = evbag_max();
    struct epoll_event events[evmax];
    struct epoll_event ev;
    struct evbag *bag;
    struct circuitA *c;
    struct evstate *s;
    int i;
    int nfds;
    int fd;
    int ret = OK;

    while (((status == NULL) || (*status > EXIT_FAILURE)) && evbag_count()) {
        nfds = epoll_wait(epollfd, events, evmax, -1);
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
            c = bag->circuit;
            s = bag->state;

            s->events = ev.events;
            continueA(c, bag->current, s);
        }
    }

    return ret;
}
