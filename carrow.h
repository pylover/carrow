#ifndef CARROW_H
#define CARROW_H


#include <stdbool.h>
#include <sys/epoll.h>


/* Generic stuff */
#define CARROW_NAME_PASTER(x, y) x ## _ ## y
#define CARROW_NAME_EVALUATOR(x, y)  CARROW_NAME_PASTER(x, y)
#define CARROW_NAME(n) CARROW_NAME_EVALUATOR(CARROW_ENTITY, n)


#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __DBG__ errno, __FILENAME__, __LINE__, __FUNCTION__


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
    enum carrow_event_status status;
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
carrow_evloop_register(void *c, void *state, struct carrow_event *e, 
        carrow_event_handler handler);

int
carrow_evloop_modify(void *c, void *state, struct carrow_event *e, 
        carrow_event_handler handler);


int
carrow_evloop_modify_or_register(void *c, void *state, struct carrow_event *e, 
        carrow_event_handler handler);


bool
carrow_evloop_isregistered(int fd);


int
carrow_evloop_unregister(int fd);


int
carrow_evloop(volatile int *status);


#endif
