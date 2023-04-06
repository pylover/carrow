#include "evbag.h"
#include "core.h"
#include "ev.h"

#include <clog.h>

#include <stdlib.h>
#include <unistd.h>


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
