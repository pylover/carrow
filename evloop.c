#include "evloop.h"

#include <clog.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>


#define EVCHUNK     128
static unsigned int _openmax;
static volatile unsigned int _bagscount;
static struct _evbag **_bags;
static int _epollfd = -1;


/* Disposables */
#define DISPOSABLESMAX   (EVCHUNK * 2)
static int _disposablescount;
static int _disposables[DISPOSABLESMAX];


struct _coro {
    void *resolve;
    void *reject;
};

    
struct _evbag {
    struct event *event;
    struct _coro coro;
    void *state;
    carrow_evhandler handler;
};


static struct _evbag *
_evbag_new(struct _coro *self, void *state, struct event *e, 
        carrow_evhandler handler) {
    int fd = e->fd;

    struct _evbag *bag = _bags[fd];
    if (bag == NULL) {
        bag = malloc(sizeof(struct _evbag));
        if (bag == NULL) {
            FATAL("Out of memory");
        }
        _bags[fd] = bag;
        _bagscount++;
        bag->coro = *self;
        bag->state = state;
        bag->event = e;
        bag->handler = handler;
    }

    return bag;
}


void
_dispose_asap(int fd) {
    if ((DISPOSABLESMAX - _disposablescount) == 1) {
        FATAL("Disposables overflow");
    }

    _disposables[_disposablescount++] = fd;
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
    
    _disposablescount = 0;
    return 0;
}


void
carrow_evloop_deinit() {
    int i;
    struct _evbag *bag;

    close(_epollfd);
    _epollfd = -1;

    for (i = 0; i < _openmax; i++) {
        bag = _bags[i];

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
carrow_arm(void *c, void *state, struct event *e, carrow_evhandler handler) {
    struct epoll_event ee;
    struct _evbag *bag = _evbag_new(c, state, e, handler);
    
    ee.events = e->op;
    int fd = e->fd;
    ee.data.fd = fd;
    
    if (epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ee)) {
        if (errno == ENOENT) {
            errno = 0;
            return epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &ee);
        }

        return -1;
    }

    return 0;
}


int
carrow_dearm(int fd) {
    struct _evbag *bag = _bags[fd];

    if (bag != NULL) {
        _bagscount--;
        _dispose_asap(fd);
    }

    return epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL);
}


int
carrow_evloop(volatile int *status) {
    int i;
    int fd;
    int nfds;
    int ret = 0;
    struct _evbag *bag;
    struct epoll_event ee;
    struct epoll_event events[EVCHUNK];

    while (((status == NULL) || (*status > EXIT_FAILURE)) && _bagscount) {
        nfds = epoll_wait(_epollfd, events, EVCHUNK, -1);
        if (nfds < 0) {
            ret = -1;
            break;
        }

        if (nfds == 0) {
            ret = 0;
            break;
        }

        for (i = 0; i < nfds; i++) {
            ee = events[i];
            fd = ee.data.fd;
            bag = _bags[fd];

            if (bag == NULL) {
                /* Event already removed */
                continue;
            }

            bag->event->op = ee.events;
            bag->handler(&(bag->coro), bag->state);
        }

        while (_disposablescount) {
            fd = _disposables[--_disposablescount];
            bag = _bags[fd];

            if (bag == NULL) {
                continue;
            }

            free(bag);
            _bags[fd] = NULL;
        }
    }

    return ret;
}
