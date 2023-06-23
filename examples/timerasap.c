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
#include <sys/timerfd.h>
#include <stdlib.h>
#include <unistd.h>

#include <clog.h>

#include "carrow.h"


typedef struct timer {
    unsigned int interval;
    unsigned long value;
    const char *title;
} timer;


#undef CARROW_ENTITY
#define CARROW_ENTITY timer
#include "carrow_generic.h"
#include "carrow_generic.c"


static int
maketimer(unsigned int interval) {
    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return -1;
    }

    struct timespec sec1 = {interval, 0};
    struct itimerspec spec = {sec1, sec1};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        return -1;
    }
    return fd;
}


static void
timer_asapfunc(int value) {
    INFO("called from asapfunction value: %d", value);
}


static void
timerA(struct timer_coro *self, struct timer *state) {
    CORO_START;
    unsigned long tmp;
    ssize_t bytes;

    self->fd = maketimer(state->interval);
    if (self->fd == -1) {
        CORO_REJECT("maketimer");
    }

    while (true) {
        CORO_WAIT(self->fd, CIN);
        bytes = read(self->fd, &tmp, sizeof(tmp));
        state->value += tmp;
        INFO("%s, fd: %d, value: %lu", state->title, self->fd, state->value);

        if (state->value % 5 == 0) {
            INFO("registering to asap");
            carrow_asap_register((carrow_asapfunc)timer_asapfunc,
                    (void *) state->value);
        }
    }

    CORO_FINALLY;
    if (self->fd != -1) {
        timer_evloop_unregister(self->fd);
        close(self->fd);
    }
    CORO_END;
}


int
main() {
    clog_verbosity = CLOG_DEBUG;

    if (carrow_handleinterrupts()) {
        return EXIT_FAILURE;
    }

    struct timer state1 = {
        .title = "Foo",
        .interval = 1,
        .value = 0,
    };
    struct timer state2 = {
        .title = "Bar",
        .interval = 3,
        .value = 0,
    };
    if (carrow_init(0, 10)) {
        ERROR("unable to init carrow");
        return EXIT_FAILURE;
    }

    INFO("CARROW INIT");
    timer_coro_create_and_run(timerA, &state1);
    //timer_coro_create_and_run(timerA, &state2);
    carrow_evloop(NULL);
    carrow_deinit();

    return EXIT_SUCCESS;
}
