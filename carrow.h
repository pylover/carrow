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
#ifndef CARROW_H_
#define CARROW_H_


#include <stdbool.h>
#include <errno.h>
#include <sys/epoll.h>


/* Coroutine */
#define CORO_START \
    if (self->events == 0) goto carrow_finally; \
    switch (self->line) { \
        case 0:


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
    } \
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


/* A struct representing a coroutine state.
   It contains the file descriptor, the event mask,
   and the line number where the coroutine was last suspended. */
struct carrow_generic_coro {
    void *run;
    int line;
    int fd;
    int events; // event mask
};

/* A function pointer type representing a coroutine handler function. */
typedef void (*carrow_generic_corofunc) (void *coro, void *state);


/* A function that initializes the event loop with the given maximum number
   of file descriptors. */
int
carrow_init(unsigned int openmax);


/* A function that frees the memory allocated for the _evbags array. */
void
carrow_deinit();

/* A function that registers a coroutine with the event loop for a given file
   descriptor and event mask. It sets the evbag for the given file descriptor
   with the provided coroutine state and handler function, and adds the file
   descriptor to the epoll instance. */
int
carrow_evloop_register(void *coro, void *state, int fd, int events,
        carrow_generic_corofunc handler);

/* A function that modifies the coroutine state and handler function for a
   given file descriptor in the event loop. */
int
carrow_evloop_modify(void *c, void *state, int fd, int events,
        carrow_generic_corofunc handler);


/* A function that modifies the coroutine state and handler function for a
   given file descriptor in the event loop if it exists, or registers a new
   coroutine with the provided state and handler function if it does not. */
int
carrow_evloop_modify_or_register(void *coro, void *state, int fd, int events,
        carrow_generic_corofunc handler);

/* A function that unregisters a coroutine for a given file descriptor from
   the event loop and deletes its evbag. */
int
carrow_evloop_unregister(int fd);

/* A function that runs the event loop until the _evbagscount is zero or
   the status is set to a value less than or equal to EXIT_FAILURE.
   It waits for events on the epoll instance and triggers thecorresponding
   coroutines when events occur. */
int
carrow_evloop(volatile int *status);

/* A function that set the coroutine state and coroutine handler for the 
   given file descriptor. */
int
carrow_evbag_unpack(int fd, struct carrow_generic_coro *coro, void **state,
        carrow_generic_corofunc *handler);


/* A function that installs a signal handler for SIGINT that sets the
   _carrow_status variable to EXIT_SUCCESS.*/
int
carrow_handleinterrupts();


/*  A function that creates a timer file descriptor and sets it to expire
    after the given number of seconds. It sets the coroutine state to wait
    on the timer file descriptor and returns the line number where the
    coroutine was suspended. */
int
carrow_sleep(struct carrow_generic_coro *self, unsigned int seconds,
        int line);


/* A function that triggers the coroutine associated with the given file
   descriptor with the provided event mask. */
int
carrow_trigger(int fd, int events);


#endif  // CARROW_H_
