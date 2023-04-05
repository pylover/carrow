#ifndef CARROW_EVBAG_H
#define CARROW_EVBAG_H


#include <stdbool.h>


struct evbag {
    struct elementA *current;
    struct evstate *state;
};


int
evbag_init();


void
evbags_deinit();


unsigned int
evbag_max();


unsigned int
evbag_count();


struct evbag *
evbag_get(int fd);


void
evbag_free(int fd);


struct evbag *
bag_new(struct evstate *state);


struct evbag *
evbag_ensure(struct circuitA *c, struct evstate *state);


#endif
