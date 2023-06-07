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
#ifndef CARROW_H_  // NOLINT(build/header_guard)
#error "carrow.h and clog.h must be imported before importing the" \
    "carrow_generic.h"
#error "And also #undef and #define CARROW_ENTITY before importing the " \
    "carrow_generic.h"
#else


#include <stdbool.h>


typedef struct CARROW_NAME(coro) CARROW_NAME(coro);


typedef void (*CARROW_NAME(corofunc)) (CARROW_NAME(coro)*,
        CARROW_ENTITY *state);


struct CARROW_NAME(coro) {
    CARROW_NAME(corofunc) run;
    int line;
    int fd;
    int events;
};


CARROW_NAME(coro)
CARROW_NAME(coro_create) (CARROW_NAME(corofunc) f);


CARROW_NAME(coro)
CARROW_NAME(coro_stop) ();


void
CARROW_NAME(coro_run) (CARROW_NAME(coro) *self, CARROW_ENTITY *s);


void
CARROW_NAME(coro_create_and_run) (CARROW_NAME(corofunc), CARROW_ENTITY *);


int
CARROW_NAME(evloop_register) (CARROW_NAME(coro) *c, CARROW_ENTITY *s,
        int fd, int events);


int
CARROW_NAME(evloop_modify) (CARROW_NAME(coro) *c, CARROW_ENTITY *s, int fd,
        int events);


int
CARROW_NAME(evloop_modify_or_register) (CARROW_NAME(coro) *c,
        CARROW_ENTITY *s, int fd, int events);


int
CARROW_NAME(evloop_unregister) (int fd);


int
CARROW_NAME(forever) (CARROW_NAME(corofunc) resolve, CARROW_ENTITY *state,
        volatile int *status);


#endif
