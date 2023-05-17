#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/timerfd.h>


/* TCP server carrow types and function */
struct state {
    int fd;
    unsigned long value;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   struct state
#define CCORO    timer
#define CNAME(n) timer_ ## n
#include "carrow.h"
#include "carrow.c"


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;
static struct carrow_event ev;


struct timer 
errorA(struct timer *self, struct state *state, int no) {
    if (state->fd != -1) {
        timer_nowait(state->fd);
        close(state->fd);
    }
    return timer_stop();
}


struct timer 
hitA(struct timer *self, struct state *state) {
    unsigned long value;
    ssize_t bytes = read(state->fd, &value, sizeof(value));
    if (bytes == 0) {
        return timer_reject(self, state, DBG, "read: EOF");
    }

    if (bytes == -1) {
        if (!EVMUSTWAIT()) {
            return timer_reject(self, state, DBG, "read");
        }

        if (timer_wait(self, state, &ev, state->fd, EVIN | EVONESHOT)) {
            return timer_reject(self, state, DBG, "timer_wait");
        }
        return timer_stop();
    }
    
    state->value += value;
    INFO("hit, fd: %d, value: %lu", state->fd, state->value);
    return *self;
}


struct timer 
timerA(struct timer *self, struct state *state) {
    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return timer_reject(self, state, DBG, "timerfd_create");
    }

    state->fd = fd;
    struct timespec sec1 = {1, 0};
    struct itimerspec spec = {sec1, sec1};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        return timer_reject(self, state, DBG, "timerfd_settime");
    }
    
    return timer_from(self, hitA);
}


void sighandler(int s) {
    PRINTE(CR);
    status = EXIT_SUCCESS;
}


void catch_signal() {
    struct sigaction new_action = {sighandler, 0, 0, 0, 0};
    if (sigaction(SIGINT, &new_action, &old_action) != 0) {
        FATAL("sigaction");
    }
}


int
main() {
    int ret = EXIT_SUCCESS;
    clog_verbosity = CLOG_DEBUG;

    /* Signal */
    catch_signal();
    
    /* Create timer */
    struct state state = {
        .fd = -1,
    };
    
    carrow_init();
    if (timer_runloop(timerA, errorA, &state, &status)) {
        ret = EXIT_FAILURE;
    }
    
    carrow_deinit();
    return ret;
}
