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


struct unixs 
errorA(struct unixs *self, struct state *state, int no) {
    unixs_nowait(state->listenfd);
    close(state->listenfd);
    return unixs_stop();
}


struct unixs
acceptA(struct unixs *self, struct state *state) {
    return unixs_stop();
}


struct unixs 
listenA(struct unixs *self, struct state *state) {
    int fd;

    fd = unix_listen(state->sockfile);
    if (fd < 0) {
        goto failed;
    }
    
    state->listenfd = fd;
    return unixs_from(self, acceptA);

failed:
    return unixs_reject(self, state, DBG, "unix_listen(%s)", state->sockfile);
}


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
    
    ev_init();
    if (unixs_runloop(listenA, errorA, &state, &status)) {
        ret = EXIT_FAILURE;
    }

    ev_deinit();
    return ret;
}
