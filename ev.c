#include "evbag.h"
#include "ev.h"

#include <clog.h>

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>


static int epollfd = -1;


static int
_evarm(struct evbag *bag, int op) {
    struct epoll_event ev;
    ev.events = op;
    ev.data.ptr = bag;
    
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, bag->state->fd, &ev)) {
        if (errno == ENOENT) {
            errno = 0;
            return epoll_ctl(epollfd, EPOLL_CTL_ADD, bag->state->fd, &ev);
        }
        return -1;
    }

    return 0;
}


int
evdearm(int fd) {
    evbag_free(fd);
    return epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}


struct elementA * 
evwaitA(struct circuitA *c, struct evstate *s, int fd, int op) {
    s->fd = fd;
    struct evbag *bag = evbag_ensure(c, s);

    /* Arm */
    if (_evarm(bag, op)) {
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
    errno = 0;
    
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
            fd = s->fd;

            s->events = ev.events;
            DEBUG("Continue");
            enum circuitstatus st = continueA(c, bag->current, s);
            switch (st) {
                case CSDISPOSE:
                    DEBUG("DISPOSE");
                    evdearm(fd);
                    break;
                case CSSTOP:
                    DEBUG("STOP");
                    evdearm(fd);
                    break;
                case CSOK:
                    DEBUG("OK");
                    break;
            }
        }
    }

    errno = 0;
    return ret;
}
