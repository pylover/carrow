#ifndef CARROW_EVLOOP_H
#define CARROW_EVLOOP_H


#include <sys/epoll.h>


#define EVMUSTWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK))
#define EVIN      EPOLLIN
#define EVOUT     EPOLLOUT
#define EVET      EPOLLET
#define EVONESHOT EPOLLONESHOT


struct event {
    int fd;
    int op;
};


typedef void (*carrow_evhandler) (void *coro, void *state);


int
carrow_arm(void *c, void *state, struct event *e, carrow_evhandler handler);


int
carrow_dearm(int fd);


int
carrow_evloop_init();


void
carrow_evloop_deinit();


int
carrow_evloop(volatile int *status);


#endif