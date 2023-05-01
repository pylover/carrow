#include "unix.h"
#include "tty.h"

#include <mrb.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


/* TCP server carrow types and function */
struct state {
    int listenfd;

    const char *sockfile;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   struct state
#define CCORO    unixs
#define CNAME(n) unixs_ ## n
#include "carrow.c"


/* TCP connection carrow types and function */
struct connstate {
    struct unixconn conn;
    mrb_t buff;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   struct connstate
#define CCORO    unixsc
#define CNAME(n) unixsc_ ## n
#include "carrow.c"


#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;
static struct event ev = {
    .fd = STDIN_FILENO,
    .op = EVIN,
};


static void 
sighandler(int s) {
    PRINTE(CR);
    status = EXIT_SUCCESS;
}


static void 
catch_signal() {
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

    struct state state = {
        .sockfile = "foo",
        .listenfd = -1,
    };
    
    carrow_evloop_init();
    // if (unixs_runloop(listenA, errorA, &state, &status)) {
    //     ret = EXIT_FAILURE;
    // }

    carrow_evloop_deinit();
    return ret;
}
