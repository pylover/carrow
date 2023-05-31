#include "carrow.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/timerfd.h>


typedef struct timer {
    int interval;
    int line;
    unsigned long value;
} timer;


#undef CARROW_ENTITY
#define CARROW_ENTITY timer
#include "carrow_generic.h"
#include "carrow_generic.c"


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;


#define CORO_START if (events == 0) goto finish; \
    switch(state->line) { case 0:
#define CORO_REJECT(...) ERROR(__VA_ARGS__); goto finish;
#define CORO_FINISH }; finish:
#define CORO_WAIT() do { state->line = __LINE__; return timer_coro_stop(); \
    case __LINE__:} while (0)


struct timer_coro 
timerA(struct timer_coro *self, struct timer *state, int fd, int events) {
    CORO_START;
    unsigned long tmp;
    ssize_t bytes;

    fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        CORO_REJECT("timerfd_create");
    }
    
    struct timespec sec1 = {state->interval, 0};
    struct itimerspec spec = {sec1, sec1};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        CORO_REJECT("timerfd_settime");
    }
    
    if (timer_evloop_register(self, state, fd, CIN)) {
        CORO_REJECT("timer_wait");
    }

    while (true) {
        CORO_WAIT();
        bytes = read(fd, &tmp, sizeof(tmp));
        state->value += tmp;
        INFO("hit, fd: %d, value: %lu", fd, state->value);

        CORO_WAIT();
        bytes = read(fd, &tmp, sizeof(tmp));
        state->value += tmp;
        INFO("hit, fd: %d, value: %lu", fd, state->value);
    }

    CORO_FINISH;
    if (fd != -1) {
        timer_evloop_unregister(fd);
        close(fd);
    }
    return timer_coro_stop();
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
    clog_verbosity = CLOG_DEBUG;
    catch_signal();
    struct timer state1 = {
        .interval = 1,
        .line = 0,
        .value = 0,
    };
    struct timer state2 = {
        .interval = 3,
        .line = 0,
        .value = 0,
    };

    carrow_init(0);
    timer_coro_create_and_run(timerA, NULL, &state1);
    timer_coro_create_and_run(timerA, NULL, &state2);
    carrow_evloop(&status);
    carrow_deinit();
}
