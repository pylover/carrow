#include "carrow.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/timerfd.h>


/* timer types and function */
typedef struct timer {
    int fd;
    unsigned long value;
} timer;


#undef CARROW_ENTITY
#define CARROW_ENTITY timer
#include "carrow_generic.h"
#include "carrow_generic.c"


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;


struct timer_coro 
errorA(struct timer_coro *self, struct timer *state, int no) {
    if (state->fd != -1) {
        timer_evloop_unregister(state->fd);
        close(state->fd);
    }
    return timer_coro_stop();
}


struct timer_coro 
hitA(struct timer_coro *self, struct timer *state, int fd, int events) {
    unsigned long value;

    if (events == 0) {
        return timer_coro_reject(self, state, __DBG__, "terminating");
    }
    ssize_t bytes = read(state->fd, &value, sizeof(value));
    if (bytes == 0) {
        return timer_coro_reject(self, state, __DBG__, "read: EOF");
    }

    if (bytes == -1) {
        if (!EVMUSTWAIT()) {
            return timer_coro_reject(self, state, __DBG__, "read");
        }

        if (timer_evloop_modify_or_register(self, state, state->fd, 
                    EVIN | EVONESHOT)) {
            return timer_coro_reject(self, state, __DBG__, "timer_wait");
        }
        return timer_coro_stop();
    }
    
    state->value += value;
    INFO("hit, fd: %d, value: %lu", state->fd, state->value);
    return *self;
}


struct timer_coro 
timerA(struct timer_coro *self, struct timer *state, int fd, int events) {
    fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return timer_coro_reject(self, state, __DBG__, "timerfd_create");
    }

    state->fd = fd;
    struct timespec sec1 = {1, 0};
    struct itimerspec spec = {sec1, sec1};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        return timer_coro_reject(self, state, __DBG__, "timerfd_settime");
    }
    
    return timer_coro_create_from(self, hitA);
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
    struct timer state = {
        .fd = -1,
    };
    
    carrow_init();

    timer_coro_create_and_run(timerA, errorA, &state, -1, -1);
    if (carrow_evloop(&status)) {
        ret = EXIT_FAILURE;
    }
    
    carrow_deinit();
    return ret;
}
