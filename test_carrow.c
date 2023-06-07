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

#include <cutest.h>

#include "carrow.h"


typedef struct foo {
    int all;
    int foo;
    int bar;
} foo;


#undef CARROW_ENTITY
#define CARROW_ENTITY foo
#include "carrow_generic.h"  // NOLINT
#include "carrow_generic.c"  // NOLINT


void
barA(struct foo_coro *self, struct foo *s);


void
fooA(struct foo_coro *self, struct foo *s) {
    if (s->all >= 10) {
        self->run = NULL;
        return;
    }
    s->foo++;
    s->all++;
    self->run = barA;
}


void
barA(struct foo_coro *self, struct foo *s) {
    s->bar++;
    s->all++;
    self->run = fooA;
}


void
test_foo_loop() {
    static char b[256];
    struct foo state = {0, 0, 0};

    foo_coro_create_and_run(fooA, &state);
    eqint(10, state.all);
    eqint(5, state.foo);
    eqint(5, state.bar);
}


void main() {
    test_foo_loop();
}
