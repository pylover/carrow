// Copyright 2023 Vahid Mardani
/*
 * This file is part of Carrow.
 *  Carrow is free software: you can redistribute it and/or modify it under 
 *  the terms of the GNU General Public License as published by the Free 
 *  Software Foundation, either version 3 of the License, or (at your option) 
 *  any later version.
 *  
 *  Carrow is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along 
 *  with Carrow. If not, see <https://www.gnu.org/licenses/>. 
 *  
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#include <stdlib.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include <clog.h>

#include "carrow.h"


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

    return foo_forever(fooA, &state, NULL, 0);
}
