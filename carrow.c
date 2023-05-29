#include "carrow.h"

#include <clog.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>


#define EVCHUNK     128
static unsigned int _openmax;
static struct evbag **evbags;
volatile unsigned int evbags_count;
static int _epollfd = -1;


struct evbag {
    struct generic_coro coro;
    void *state;
    carrow_event_handler handler;
};



#define evbag_has(fd) (evbags[fd] != NULL)
#define evbag_isnull(fd) (evbags[fd] == NULL)


static int
evbag_set(int fd, struct generic_coro *coro, void *state,
        carrow_event_handler handler) {
    struct evbag *bag = evbags[fd];

    if (bag == NULL) {
        bag = malloc(sizeof(struct evbag));
        if (bag == NULL) {
            ERROR("Out of memory");
            errno = ENOMEM;
            return -1;
        }
        evbags[fd] = bag;
        evbags_count++;
    }
    bag->coro = *coro;
    bag->state = state;
    bag->handler = handler;

    return 0;
}


static int
evbag_delete(int fd) {
    struct evbag *bag = evbags[fd];

    if (bag == NULL) {
        return -1;
    }
    
    free(bag);
    evbags_count--;
    evbags[fd] = NULL; 
    return 0;
}


int
carrow_evbag_unpack(int fd, struct generic_coro *coro, void **state, 
        carrow_event_handler *handler) {
    struct evbag *bag = evbags[fd];

    if (bag == NULL) {
        return -1;
    }

    memcpy(coro, &bag->coro, sizeof(struct generic_coro));
    *state = bag->state;
    *handler = bag->handler;
   
    return 0;
}


void
carrow_evbag_handle(int fd, int events, struct generic_coro *coro) {
    struct evbag *bag = evbags[fd];
    struct generic_coro *c = coro;
    
    if (bag == NULL) {
        /* Event already removed */
        return;
    }
    
    if (c == NULL) {
        c = &(bag->coro);
    }

    bag->handler(c, bag->state, fd, events);
}


static int
evbags_init(unsigned int openmax) {
    evbags_count = 0;
    evbags = calloc(openmax, sizeof(struct evbag*));
    
    if (evbags == NULL) {
        errno = ENOMEM;
        ERROR("Out of memory");
        return -1;
    }
    
    memset(evbags, 0, sizeof(struct evbag*) * openmax);
    return 0;
}


static void
evbags_deinit() {
    free(evbags);
    evbags = NULL;
}


int
carrow_init() {
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
    evbags_init(_openmax);

    _epollfd = epoll_create1(0);
    if (_epollfd < 0) {
        return -1;
    }
    
    return 0;
}


int
carrow_evloop_register(void *coro, void *state, int fd, int events, 
        carrow_event_handler handler) {
    struct epoll_event ee;

    if (evbag_has(fd)) {
        return -1;
    }

    evbag_set(fd, (struct generic_coro*)coro, state, handler);
    ee.events = events;
    ee.data.fd = fd;
    if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &ee)) {
        evbag_delete(fd);
        return -1;
    }

    return 0;
}


int
carrow_evloop_modify(void *coro, void *state, int fd, int events, 
        carrow_event_handler handler) {
    struct epoll_event ee;
   
    if (evbag_isnull(fd)) {
        return -1;
    }

    if (evbag_set(fd, coro, state, handler)) {
        return -1;
    }

    ee.events = events;
    ee.data.fd = fd;
    if (epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ee)) {
        evbag_delete(fd);
        return -1;
    }

    return 0;
}


int
carrow_evloop_modify_or_register(void *coro, void *state, int fd, int events, 
        carrow_event_handler handler) {
    
    if (evbag_isnull(fd)) {
        return carrow_evloop_register(coro, state, fd, events, handler);
    }
    
    return carrow_evloop_modify(coro, state, fd, events, handler);
}


int
carrow_evloop_unregister(int fd) {
    epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL);
    return evbag_delete(fd);
}


int
carrow_evloop(volatile int *status) {
    int i;
    int fd;
    int nfds;
    int ret = 0;
    struct evbag *bag;
    struct epoll_event ee;
    struct epoll_event events[EVCHUNK];

evloop:
    while (evbags_count && ((status == NULL) || (*status > EXIT_FAILURE))) {
        errno = 0;
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
            carrow_evbag_handle(fd, ee.events, NULL);
        }
    }

terminate:

    for (fd = 0; fd < _openmax; fd++) {
        carrow_evbag_handle(fd, 0, NULL);
    }

    if (evbags_count && ((status == NULL) || (*status > EXIT_FAILURE))) {
        goto evloop;
    }

    return ret;
}


void
carrow_deinit() {
    close(_epollfd);
    _epollfd = -1;
    evbags_deinit();
    errno = 0;
}
