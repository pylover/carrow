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
#include "carrow.h"  // NOLINT

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


CARROW_NAME(coro)
CARROW_NAME(coro_create) (CARROW_NAME(corofunc) f) {
    return (CARROW_NAME(coro)){f, 0, -1, -1};  // NOLINT
}


CARROW_NAME(coro)
CARROW_NAME(coro_stop) () {
    return (CARROW_NAME(coro)){NULL, -1, -1, -1};  // NOLINT
}


void
CARROW_NAME(coro_run) (CARROW_NAME(coro) *self, CARROW_ENTITY *state) {
    if (self->run == NULL) {
        return;
    }

run:
    self->run(self, state);
    if (self->run == NULL) {
        return;
    }

    if (self->fd == -1) {
        goto run;
    }

    if (CARROW_NAME(evloop_modify_or_register)(self, state, self->fd,
                self->events)) {
        self->events = 0;
        goto run;
    }
}


void
CARROW_NAME(coro_create_and_run) (CARROW_NAME(corofunc) f,
        CARROW_ENTITY *state) {
    CARROW_NAME(coro) c = CARROW_NAME(coro_create)(f);
    CARROW_NAME(coro_run)(&c, state);
}


int
CARROW_NAME(evloop_register) (CARROW_NAME(coro) *c, CARROW_ENTITY *s, int fd,
        int events) {
    return carrow_evloop_register(c, s, fd, events,
            (carrow_generic_corofunc)CARROW_NAME(coro_run));
}


int
CARROW_NAME(evloop_modify) (CARROW_NAME(coro) *c, CARROW_ENTITY *s, int fd,
        int events) {
    return carrow_evloop_modify(c, s, fd, events,
            (carrow_generic_corofunc)CARROW_NAME(coro_run));
}


int
CARROW_NAME(evloop_modify_or_register) (CARROW_NAME(coro) *c,
        CARROW_ENTITY *s, int fd, int events) {
    return carrow_evloop_modify_or_register(c, s, fd, events,
            (carrow_generic_corofunc)CARROW_NAME(coro_run));
}


int
CARROW_NAME(evloop_unregister) (int fd) {
    return carrow_evloop_unregister(fd);
}


int
CARROW_NAME(forever) (CARROW_NAME(corofunc) f, CARROW_ENTITY *state,
        volatile int *status) {
    int ret = EXIT_SUCCESS;

    carrow_init(0);
    CARROW_NAME(coro_create_and_run) (f, state);
    if (carrow_evloop(status)) {
        ret = EXIT_FAILURE;
    }

    carrow_deinit();
    return ret;
}
