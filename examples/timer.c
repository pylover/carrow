#include "carrow.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/timerfd.h>


/* timer types and function */
// typedef struct timer {
//     int fd;
// } timer;


typedef int timer;


#undef CARROW_ENTITY
#define CARROW_ENTITY timer
#include "carrow_generic.h"
#include "carrow_generic.c"


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;


struct timer_coro 
errorA(struct timer_coro *self, int *fdptr, int no) {
    int fd = *fdptr;
    if (fd != -1) {
        timer_evloop_unregister(fd);
        close(fd);
    }
    return timer_coro_stop();
}


#define CORO_START() if (events == 0) goto terminate
#define CORO_JUMP(c, l) else if (c) goto l
#define CORO_WAIT(l) return timer_coro_stop(); l:
#define CORO_FINISH() terminate: \
    return timer_coro_reject(self, fdptr, CDBG, "terminating")


struct timer_coro 
timerA(struct timer_coro *self, int *fdptr, int fd, int events) {
    CORO_START();
    CORO_JUMP(fd != -1, tick);

    static unsigned long value = 0;
    unsigned long tmp;
    ssize_t bytes;

    fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return timer_coro_reject(self, fdptr, CDBG, "timerfd_create");
    }
    
    *fdptr = fd;
    struct timespec sec1 = {1, 0};
    struct itimerspec spec = {sec1, sec1};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        return timer_coro_reject(self, fdptr, CDBG, "timerfd_settime");
    }
    
    if (timer_evloop_register(self, fdptr, fd, CIN)) {
        return timer_coro_reject(self, fdptr, CDBG, "timer_wait");
    }

    while (true) {
        CORO_WAIT(tick);
        bytes = read(fd, &tmp, sizeof(tmp));
        if (bytes == 0) {
            return timer_coro_reject(self, fdptr, CDBG, "read: EOF");
        }
        value += tmp;
        INFO("hit, fd: %d, value: %lu", fd, value);
    }

    CORO_FINISH();
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
    
    int fd = -1;
    return timer_forever(timerA, errorA, &fd, &status);
}
