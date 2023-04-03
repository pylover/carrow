#include "carrow.h"
#include "tcp.h"
#include "addr.h"

#include <clog.h>
#include <mrb.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


#define WORKING 9999

#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


static volatile int status = WORKING;
static struct sigaction old_action;


struct connstate {
    struct tcpclientstate;

    struct mrb inbuff;
    struct mrb outbuff;

    size_t bytesin;
    size_t bytesout;
};


struct elementA * 
_conn_error(struct circuitA *c, struct connstate *s, const char *msg) {
    DEBUG("Closed: %d in: %lu, out: %lu: %s", s->fd, s->bytesin, s->bytesout,
            msg);
    return stopA(c);
}


struct elementA *
_pipeA(struct circuitA *c, struct connstate *s) {
    // DEBUG("Wait: fd: %d in: %lu, out: %lu", fd, s->bytesin, s->bytesout);
    // return evwaitA(c, (struct evstate*)s, fd, want | EVONESHOT);

    return nextA(c, s);
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

    struct connstate state = {
        .host = "0.0.0.0",
        .port = "3030",
    };
    struct circuitA *c = NEW_A(NULL);
                         APPEND_A(c, evinitA, NULL);
                         APPEND_A(c, connectA, NULL);
    struct elementA *e = APPEND_A(c, _pipeA, NULL);
               loopA(e); 

    runA(c, &state);
    if (evloop(&status)) {
        ret = EXIT_FAILURE;
    }
    evdeinitA();
    freeA(c);
    return ret;
}
