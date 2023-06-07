// copyright 2023 vahid mardani
/*
 * this file is part of carrow.
 *  carrow is free software: you can redistribute it and/or modify it under 
 *  the terms of the gnu general public license as published by the free 
 *  software foundation, either version 3 of the license, or (at your option) 
 *  any later version.
 *  
 *  carrow is distributed in the hope that it will be useful, but without any 
 *  warranty; without even the implied warranty of merchantability or fitness 
 *  for a particular purpose. see the gnu general public license for more 
 *  details.
 *  
 *  you should have received a copy of the gnu general public license along 
 *  with carrow. if not, see <https://www.gnu.org/licenses/>. 
 *  
 *  author: vahid mardani <vahid.mardani@gmail.com>
 */
#include "carrow.h"

#include <clog.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/timerfd.h>


typedef struct foo {
    unsigned int counter;
} foo;


#undef CARROW_ENTITY
#define CARROW_ENTITY foo
#include "carrow_generic.h"
#include "carrow_generic.c"


static void
fooA(struct foo_coro *self, struct foo *state) {
    CORO_START;

    while (true) {
        INFO("Flip: %u", state->counter++);
        CORO_SLEEP(1);

        INFO("Flop: %u", state->counter++);
        CORO_SLEEP(1);
    }

    CORO_FINALLY;
    CORO_CLEANUP;
}


int
main() {
    clog_verbosity = CLOG_DEBUG;

    if (carrow_handleinterrupts()) {
        return EXIT_FAILURE;
    }

    struct foo state = {
        .counter = 0
    };

    return foo_forever(fooA, &state, NULL);
}
