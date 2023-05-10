#ifndef CARROW_EVLOOP_H
#define CARROW_EVLOOP_H


#include <sys/epoll.h>


#define EVMUSTWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK) \
        || (errno == EINPROGRESS))
#define EVIN      EPOLLIN
#define EVOUT     EPOLLOUT
#define EVET      EPOLLET
#define EVONESHOT EPOLLONESHOT


enum ev_status {
    EE_OK,
    EE_ERR,
    EE_TERM,
};


struct event {
    int fd;
    int op;
};


typedef void (*ev_handler) (void *coro, void *state, enum ev_status);


int
ev_wait(void *c, void *state, struct event *e, ev_handler handler);


int
ev_nowait(int fd);


int
ev_init();


void
ev_deinit();


int
ev_loop(volatile int *status);


#endif
