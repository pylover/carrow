#ifndef CARROWEV_H
#define CARROWEV_H


#include <sys/epoll.h>


#define EVMUSTWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK) \
        || (errno == EINPROGRESS))
#define EVIN      EPOLLIN
#define EVOUT     EPOLLOUT
#define EVET      EPOLLET
#define EVONESHOT EPOLLONESHOT


enum carrow_evstatus {
    EE_OK,
    EE_ERR,
    EE_TERM,
};


struct carrow_event {
    int fd;
    int op;
};


typedef void (*carrow_evhandler) 
    (void *coro, void *state, enum carrow_evstatus);


int
carrow_wait(void *c, void *state, struct carrow_event *e, 
        carrow_evhandler handler);


int
carrow_nowait(int fd);


int
carrow_init();


void
carrow_deinit();


int
carrow_evloop(volatile int *status);


#endif
