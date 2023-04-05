#include "evbag.h"
#include "core.h"
#include "ev.h"

#include <clog.h>

#include <stdlib.h>
#include <unistd.h>


static unsigned int openmax;
static unsigned int bagscount;
static struct evbag **bags;


int
evbag_init() {
    openmax = sysconf(_SC_OPEN_MAX);
    if (openmax == -1) {
        ERROR("sysconf(openmax)");
        return -1;
    }

    bagscount = 0;
    bags = calloc(openmax, sizeof(struct evbag*));
    if (bags == NULL) {
        FATAL("Out of memory");
    }

    return 0;
}


void
evbags_deinit() {
    free(bags);
    bags = NULL;
}


unsigned int
evbag_max() {
    return openmax;
}


unsigned int
evbag_count() {
    return bagscount;
}


struct evbag *
evbag_get(int fd) {
    return bags[fd];
}


void
evbag_free(int fd) {
    struct evbag *bag = bags[fd];
    if (bag == NULL) {
        return;
    }
    free(bag);
    bags[fd] = NULL;
    bagscount--;
}


struct evbag *
evbag_ensure(struct evstate *state) {
    int fd = state->fd;
    struct evbag *bag = bags[fd];

    if (bag == NULL) {
        bag = malloc(sizeof(struct evbag));
        if (bag == NULL) {
            FATAL("Out of memory");
        }
        bags[fd] = bag;
        bagscount++;
    }
    
    bag->current = currentA(c);
    bag->state = state;
    return bag;
}
