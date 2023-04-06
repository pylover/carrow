#include "evloop.h"

#include <clog.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>


/* Event loop */
static unsigned int _openmax;
static unsigned int _bagscount;
static struct _evbag **_bags;
static int _epollfd;


struct _coro {
    void *resolve;
    void *reject;
};


struct _evbag {
    struct _coro coro;
    void *state;
    int fd;
};


static struct _evbag *
_evbag_new (struct _coro *self, void *state, int fd) {
    struct _evbag *bag = malloc(sizeof(struct _evbag));
    if (bag == NULL) {
        FATAL("Out of memory");
    }

    bag->coro = *self;
    bag->state = state;
    bag->fd = fd;
    return bag;
}


static struct _evbag *
_evbag_get(int fd) {
    return _bags[fd];
}


int
carrow_arm(void *c, void *state, int fd, int op) {
    struct epoll_event ev;
    struct _evbag *bag = _evbag_new(c, state, fd);
    
    ev.events = op;
    ev.data.ptr = bag;
    
    if (epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev)) {
        if (errno == ENOENT) {
            errno = 0;
            return epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &ev);
        }

        return -1;
    }

    return 0;
}


int
carrow_dearm(int fd) {
    return epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, NULL);
}


int
carrow_evloop_init() {
    if (_epollfd != -1 ) {
        ERROR("Carrow event loop already initialized.");
        return -1;
    }
   
    /* Find maximum allowed openfiles */
    _openmax = sysconf(_SC_OPEN_MAX);
    if (_openmax == -1) {
        ERROR("Cannot get maximum allowed openfiles for this process.");
        return -1;
    }

    _bagscount = 0;
    _bags = calloc(_openmax, sizeof(struct _evbag*));
    if (_bags == NULL) {
        FATAL("Out of memory");
    }

    _epollfd = epoll_create1(0);
    if (_epollfd < 0) {
        return -1;
    }

    return 0;
}


void
carrow_evloop_deinit() {
    int i;
    struct _evbag *bag;

    close(_epollfd);
    _epollfd = -1;

    for (i = 0; i < _openmax; i++) {
        bag = _evbag_get(i);

        if (bag == NULL) {
            continue;
        }

        carrow_dearm(i);
    }
    
    free(_bags);
    _bags = NULL;
    errno = 0;
}


int
carrow_evloop (volatile int *status) {
    int i;
    int nfds;
    int ret = 0;
    // struct epoll_event ev;
    // struct epoll_event events[EVMAX];

    // while ((status == NULL) || (*status > EXIT_FAILURE)) {
    //     nfds = epoll_wait(_epollfd, events, evmax, -1);
    //     if (nfds < 0) {
    //         ret = -1;
    //         break;
    //     }

    //     if (nfds == 0) {
    //         ret = 0;
    //         break;
    //     }

    //     for (i = 0; i < nfds; i++) {
    // //         ev = events[i];
    // //         bag = (struct evbag *) ev.data.ptr;
    //     }
    // }

    return ret;
}
