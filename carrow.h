// copyright 2023 vahid mardani
/*
 * this file is part of carrow.
 *  carrow is free software: you can redistribute it and/or modify it under 
 *  the terms of the gnu general public license as published by the free 
 *  software foundation, either version 3 of the license, or (at your option) 
 *  any later version.
 *  
 *  carrow is distributed in the hope that it will be useful, but without any 
 *  warranty; without even the implied warranty of merchantability or fitness 
 *  for a particular purpose. see the gnu general public license for more 
 *  details.
 *  
 *  you should have received a copy of the gnu general public license along 
 *  with carrow. if not, see <https://www.gnu.org/licenses/>. 
 *  
 *  author: vahid mardani <vahid.mardani@gmail.com>
 */
#ifndef CARROW_H
#define CARROW_H


#include <stdbool.h>
#include <errno.h>
#include <sys/epoll.h>


/* Coroutine */
#define CORO_START \
    if (self->events == 0) goto carrow_finally; \
    switch (self->line) { case 0:


#define CORO_REJECT(...) \
    ERROR(__VA_ARGS__); \
    goto carrow_finally


#define CORO_WAIT(f, e) \
    do { \
        self->line = __LINE__; \
        self->fd = f; \
        self->events = e | CONCE; \
        return; \
        case __LINE__:; \
    } while (0)


#define CORO_SLEEP(sec) \
    do { \
        if (carrow_sleep((struct carrow_generic_coro*)self, sec, __LINE__)) \
            goto carrow_finally;\
        return; \
        case __LINE__: \
        carrow_evloop_unregister(self->fd); \
        close(self->fd); \
        self->fd = -1; \
    } while (0)


#define CORO_FINALLY \
    }; \
    carrow_finally:


#define CORO_END \
    self->run = NULL; \
    return


#define CORO_NEXT(f) \
    do { \
        self->run = f; \
        self->fd = -1; \
        self->line = 0; \
    } while (0)


#define CORO_CLEANUP \
    if (self->fd != -1) { \
        carrow_evloop_unregister(self->fd); \
        close(self->fd); \
    } CORO_END


/* Generic stuff */
#define CARROW_NAME_PASTER(x, y) x ## _ ## y
#define CARROW_NAME_EVALUATOR(x, y)  CARROW_NAME_PASTER(x, y)
#define CARROW_NAME(n) CARROW_NAME_EVALUATOR(CARROW_ENTITY, n)


#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


#define CDBG errno, __FILENAME__, __LINE__, __FUNCTION__
#define CMUSTWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK) \
        || (errno == EINPROGRESS))
#define CIN      EPOLLIN       // 1
#define COUT     EPOLLOUT      // 4
#define CET      EPOLLET       // -2147483648
#define CONCE    EPOLLONESHOT  // 1073741824
#define CRDHUP   EPOLLRDHUP    // 8192
#define CPRI     EPOLLPRI      // 2
#define CERR     EPOLLERR      // 8
#define CHUP     EPOLLHUP      // 16
#define CWAKEUP  EPOLLWAKEUP   // 536870912


struct carrow_generic_coro {
    void *run;
    int line;
    int fd;
    int events;
};


typedef void (*carrow_generic_corofunc) (void *coro, void *state);


int
carrow_init(unsigned int openmax);


void
carrow_deinit();


int
carrow_evloop_register(void *coro, void *state, int fd, int events,
        carrow_generic_corofunc handler);


int
carrow_evloop_modify(void *c, void *state, int fd, int events,
        carrow_generic_corofunc handler);


int
carrow_evloop_modify_or_register(void *coro, void *state, int fd, int events,
        carrow_generic_corofunc handler);


int
carrow_evloop_unregister(int fd);


int
carrow_evloop(volatile int *status);


int
carrow_evbag_unpack(int fd, struct carrow_generic_coro *coro, void **state,
        carrow_generic_corofunc *handler);


int
carrow_handleinterrupts();


int
carrow_sleep(struct carrow_generic_coro *self, unsigned int seconds,
        int line);


int
carrow_trigger(int fd, int events);


#endif
