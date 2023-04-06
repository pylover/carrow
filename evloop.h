#ifndef CARROW_EVLOOP_H
#define CARROW_EVLOOP_H


typedef void (*carrow_evhandler) (void *coro, void *state, int fd, int op);


int
carrow_arm(void *c, void *state, int fd, int op, carrow_evhandler handler);


int
carrow_dearm(int fd);


int
carrow_evloop_init();


void
carrow_evloop_deinit();


int
carrow_evloop(volatile int *status);


#endif
