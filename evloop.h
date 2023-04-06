#ifndef CARROW_EVLOOP_H
#define CARROW_EVLOOP_H


int
carrow_arm(void *c, void *state, int fd, int op);


int
carrow_dearm(int fd);


int
carrow_evloop_init();


void
carrow_evloop_deinit();


#endif
