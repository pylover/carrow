#ifndef CARROW_H
#define CARROW_H


#include <stdbool.h>
#include <errno.h>
#include <sys/epoll.h>


/* Coroutine */
#define CORO_START \
    if (self->events == 0) goto carrow_finally; \
    switch(self->line) { case 0:


#define CORO_REJECT(...) \
    ERROR(__VA_ARGS__); \
    goto carrow_finally


#define CORO_WAIT(f, e) \
    do { \
        self->line = __LINE__; \
        self->fd = f; \
        self->events = e | CONCE; \
        return; \
        case __LINE__: \
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


/* Generic stuff */
#define CARROW_NAME_PASTER(x, y) x ## _ ## y
#define CARROW_NAME_EVALUATOR(x, y)  CARROW_NAME_PASTER(x, y)
#define CARROW_NAME(n) CARROW_NAME_EVALUATOR(CARROW_ENTITY, n)


#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


#define CDBG errno, __FILENAME__, __LINE__, __FUNCTION__
#define CMUSTWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK) \
        || (errno == EINPROGRESS))
#define CIN      EPOLLIN
#define COUT     EPOLLOUT
#define CET      EPOLLET
#define CONCE    EPOLLONESHOT
#define CRDHUP   EPOLLRDHUP
#define CPRI     EPOLLPRI
#define CERR     EPOLLERR
#define CHUP     EPOLLHUP
#define CWAKEUP  EPOLLWAKEUP


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


#endif
