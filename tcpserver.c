#include "carrow.h"
#include "tcp.h"
#include "addr.h"

#include <clog.h>

#include <errno.h>
#include <stdlib.h>


struct elementA *
newconnA(struct circuitA *c, struct tcpsrvstate *s) {
    INFO("New connection: %s", carrow_sockaddr_dump(&(s->newconnaddr)));
}


int
main() {
    clog_verbosity = CLOG_DEBUG;
    struct evpriv evpriv = {
        .epollflags = 0,
    };
    struct tcpsrvstate state = {
        .bindaddr = "0.0.0.0",
        .bindport = 3030,
        .backlog = 2,
    };
    struct circuitA *c = NEW_A(NULL);
                         APPEND_A(c, evinitA, &evpriv);
                         APPEND_A(c, listenA, NULL);
    struct elementA *e = APPEND_A(c, acceptA, NULL);
                         APPEND_A(c, newconnA, NULL);
               loopA(e); 

    runA(c, &state);
    if (evloop(NULL)) {
        goto failure;
    }
    evdeinitA();
    freeA(c);
    return EXIT_SUCCESS;

failure:
    evdeinitA();
    freeA(c);
    return EXIT_FAILURE;
}
