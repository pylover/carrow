#ifndef CARROW_H
#define CARROW_H


#include <sys/epoll.h>


#define EVMUSTWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK) \
        || (errno == EINPROGRESS))
#define EVIN      EPOLLIN
#define EVOUT     EPOLLOUT
#define EVET      EPOLLET
#define EVONESHOT EPOLLONESHOT
#define EVRDHUP   EPOLLRDHUP
#define EVPRI     EPOLLPRI
#define EVERR     EPOLLERR
#define EVHUP     EPOLLHUP
#define EVWAKEUP  EPOLLWAKEUP


enum carrow_event_status {
    CES_OK,
    CES_ERR,
    CES_TERM,
};


struct carrow_event {
    int fd;
    int op;
};


typedef void (*carrow_event_handler) 
    (void *coro, void *state, enum carrow_event_status);


int
carrow_init();


void
carrow_deinit();


int
carrow_evloop_add(void *c, void *state, struct carrow_event *e, 
        carrow_event_handler handler);


int
carrow_evloop_del(int fd);


int
carrow_evloop(volatile int *status);


#endif
