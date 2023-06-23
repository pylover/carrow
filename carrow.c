// Copyright 2023 Vahid Mardani
/*
 * This file is part of Carrow.
 *  Carrow is free software: you can redistribute it and/or modify it under 
 *  the terms of the GNU General Public License as published by the Free 
 *  Software Foundation, either version 3 of the License, or (at your option) 
 *  any later version.
 *  
 *  Carrow is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along 
 *  with Carrow. If not, see <https://www.gnu.org/licenses/>. 
 *  
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#include "carrow.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>


static volatile int _carrow_status = 99999;
static volatile int *_carrow_status_ptr = NULL;
static struct sigaction old_action;
static unsigned int _openmax;
static struct evbag **_evbags;
static volatile unsigned int _evbagscount;
static unsigned int _asapsmax;
static struct asapbag **_asapbags;
static volatile unsigned int _asapbagscount;
static int _epollfd = -1;


struct evbag {
    struct carrow_generic_coro coro;
    void *state;
    carrow_generic_corofunc handler;
};


struct asapbag {
    carrow_asapfunc func;
    void *params;
};


#define EVBAG_HAS(fd) (_evbags[fd] != NULL)
#define EVBAG_ISNULL(fd) (_evbags[fd] == NULL)


static void
_sighandler(int s) {
    printf("\n");
    _carrow_status = EXIT_SUCCESS;
}


static int
evbag_set(int fd, struct carrow_generic_coro *coro, void *state,
        carrow_generic_corofunc handler) {
    if (fd >= _openmax) {
        return -1;
    }
    struct evbag *bag = _evbags[fd];

    if (bag == NULL) {
        bag = malloc(sizeof(struct evbag));
        if (bag == NULL) {
            errno = ENOMEM;
            return -1;
        }
        _evbags[fd] = bag;
        _evbagscount++;
    }

    bag->coro = *coro;
    bag->state = state;
    bag->handler = handler;
    return 0;
}


static int
evbag_delete(int fd) {
    if (fd >= _openmax) {
        return -1;
    }
    struct evbag *bag = _evbags[fd];

    if (bag == NULL) {
        return -1;
    }

    free(bag);
    _evbagscount--;
    _evbags[fd] = NULL;
    return 0;
}


int
carrow_evbag_unpack(int fd, struct carrow_generic_coro *coro, void **state,
        carrow_generic_corofunc *handler) {
    if (fd >= _openmax) {
        return -1;
    }
    struct evbag *bag = _evbags[fd];

    if (bag == NULL) {
        return -1;
    }

    if (coro != NULL) {
        memcpy(coro, &bag->coro, sizeof(struct carrow_generic_coro));
    }

    if (state != NULL) {
        *state = bag->state;
    }

    if (handler != NULL) {
        *handler = bag->handler;
    }

    return 0;
}


int
carrow_trigger(int fd, int events) {
    if (fd >= _openmax) {
        return -1;
    }
    struct evbag *bag = _evbags[fd];
    struct carrow_generic_coro c;

    if (bag == NULL) {
        /* Event already removed */
        return -1;
    }

    memcpy(&c, &bag->coro, sizeof(struct carrow_generic_coro));

    c.fd = fd;
    c.events = events;
    bag->handler(&c, bag->state);
    return 0;
}


static int
evbags_init() {
    _evbagscount = 0;
    _evbags = calloc(_openmax, sizeof(struct evbag*));
    if (_evbags == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(_evbags, 0, sizeof(struct evbag*) * _openmax);
    return 0;
}


static void
evbags_deinit() {
    free(_evbags);
    _evbags = NULL;
}


static int
asapbags_init() {
    _asapbagscount = 0;
    _asapbags = calloc(_asapsmax, sizeof(struct asapbag*));
    if (_asapbags == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(_asapbags, 0, sizeof(struct asapbag*) * _asapsmax);
    return 0;
}


static void
asapbags_deinit() {
    for (int i = 0; i < _asapsmax; i++) {
        if (_asapbags[i] != NULL) {
            free(_asapbags[i]);
            _asapbags[i] = NULL;
        }
    }

    free(_asapbags);
    _asapbags = NULL;
}


static void
asap_trigger() {
    for(int i = 0; i < _asapbagscount; i++) {
        struct asapbag *bag = _asapbags[i]; 
        bag->func(bag->params);
    }
    _asapbagscount = 0;
}


int
carrow_asap_register(carrow_asapfunc func, void *params) {
    if (_asapsmax == 0) {
        return -1;
    }

    if (_asapbagscount >= _asapsmax) {
        return -1;
    }
    struct asapbag *bag = _asapbags[_asapbagscount];

    if (bag == NULL) {
        bag = malloc(sizeof(struct asapbag));
        if (bag == NULL) {
            errno = ENOMEM;
            return -1;
        }
        _asapbags[_asapbagscount] = bag;
    }

    bag->func = func;
    bag->params = params;
    _asapbagscount++;
    return 0; 
}


int
carrow_init(unsigned int openmax, unsigned int asapsmax) {
    if (_epollfd != -1) {
        return -1;
    }

    /* Find maximum allowed openfiles */
    if (openmax != 0) {
        _openmax = openmax;
    }
    else {
        _openmax = sysconf(_SC_OPEN_MAX);
        if (_openmax == -1) {
            EINVAL;
            return -1;
        }
    }

    if (evbags_init()) {
        return -1;
    }

    _asapsmax = asapsmax;
    if (_asapsmax > 0) {
        if (asapbags_init()) {
            evbags_deinit();
            return -1;
        }
    }
    
    _epollfd = epoll_create1(0);
    if (_epollfd < 0) {
        return -1;
    }

    return 0;
}


int
carrow_evloop_register(void *coro, void *state, int fd, int events,
        carrow_generic_corofunc handler) {
    struct epoll_event ee;

    if (EVBAG_HAS(fd)) {
        return -1;
    }

    evbag_set(fd, (struct carrow_generic_coro*)coro, state, handler);
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
        carrow_generic_corofunc handler) {
    struct epoll_event ee;

    if (EVBAG_ISNULL(fd)) {
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
        carrow_generic_corofunc handler) {
    if (EVBAG_ISNULL(fd)) {
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
carrow_sleep(struct carrow_generic_coro *self, unsigned int seconds,
        int line) {
    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return -1;
    }

    struct timespec sec = {seconds, 0};
    struct timespec zero = {0, 0};
    struct itimerspec spec = {zero, sec};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        close(fd);
        return -1;
    }

    self->fd = fd;
    self->events = CIN | CONCE;
    self->line = line;
    return 0;
}


int
carrow_handleinterrupts() {
    struct sigaction new_action = {_sighandler, 0, 0, 0, 0};
    if (sigaction(SIGINT, &new_action, &old_action) != 0) {
        return -1;
    }

    _carrow_status_ptr = &_carrow_status;
    return 0;
}


int
carrow_evloop(volatile int *status) {
    int i;
    int fd;
    int nfds;
    int ret = 0;
    struct evbag *bag;
    struct epoll_event ee;
    struct epoll_event events[_openmax];

    if ((status == NULL) && (_carrow_status_ptr != NULL)) {
        status = _carrow_status_ptr;
    }

evloop:
    while (_evbagscount && ((status == NULL) || (*status > EXIT_FAILURE))) {
        if (_asapbagscount > 0) {
            asap_trigger();
        }

        errno = 0;
        nfds = epoll_wait(_epollfd, events, _openmax, -1);
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
            carrow_trigger(fd, ee.events);
        }
    }

terminate:
    for (fd = 0; fd < _openmax; fd++) {
        carrow_trigger(fd, 0);
    }

    if (_evbagscount && ((status == NULL) || (*status > EXIT_FAILURE))) {
        goto evloop;
    }

    return ret;
}


void
carrow_deinit() {
    close(_epollfd);
    _epollfd = -1;
    evbags_deinit();

    if (_asapsmax > 0) {
        asapbags_deinit();
    }

    errno = 0;
}
