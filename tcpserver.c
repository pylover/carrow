#include "carrow.h"
#include "tcp.h"
#include "addr.h"

#include <clog.h>
#include <mrb.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>


struct connstate {
    int fd;
    struct sockaddr remoteaddr;
    struct mrb buff;
};


void 
_conn_error(struct circuitA *c, struct connstate *state, const char *msg) {
    ERROR("Connection error: %s", msg);
    
    close(state->fd);
    mrb_deinit(&(state->buff));
    free(state);
    freeA(c);
}


#define CHUNK   1024
#define BUFFSIZE 2048


struct elementA *
_echoA(struct circuitA *c, struct connstate *s) {
    int fd = s->fd;
    int want = 0;
    ssize_t bytes;
    struct mrb *b = &(s->buff);
    size_t toread = mrb_space_available(b);

    if (toread) {
        bytes = mrb_readin(b, fd, toread);
        if (bytes < 0) {
            if (EVMUSTWAIT()) {
                errno = 0;
                want |= EVIN;
            }
            else {
                return errorA(c, s, "read error");
            }
        }

        if (bytes == 0) {
            return errorA(c, s, "read EOF");
        }
    }

    size_t towrite = mrb_space_used(b);
    if (towrite) {
        bytes = mrb_writeout(b, fd, towrite);
        if (bytes < 0) {
            if (EVMUSTWAIT()) {
                errno = 0;
                want |= EVOUT;
            }
            else {
                return errorA(c, s, "write error");
            }
        }

        if (bytes == 0) {
            return errorA(c, s, "write EOF");
        }
    }

    if (want) {
        return evwaitA(c, (struct evstate*)s, fd, want);
    }

    return nextA(c, s);
}


struct circuitA *
_get_conncircuit() {
    struct circuitA *c = NEW_A(_conn_error);
    struct elementA *e = APPEND_A(c, _echoA, NULL);
               loopA(e); 

    return c;
}


static struct elementA *
_newconnA(struct circuitA *c, struct tcpsrvstate *s) {
    INFO("New connection: %s", carrow_sockaddr_dump(&(s->newconnaddr)));

    struct circuitA *cc = _get_conncircuit();
    struct connstate *cs = malloc(sizeof (struct connstate));
    memcpy(&(cs->remoteaddr), &(s->newconnaddr), s->addrlen);
    cs->fd = s->newconnfd; 
    if (mrb_init(&(cs->buff), BUFFSIZE)) {
        return errorA(c, s, "mrb_init");
    }
    runA(cc, cs);
}


int
main() {
    clog_verbosity = CLOG_DEBUG;
    struct tcpsrvstate state = {
        .bindaddr = "0.0.0.0",
        .bindport = 3030,
        .backlog = 2,
    };
    struct circuitA *c = NEW_A(NULL);
                         APPEND_A(c, evinitA, NULL);
                         APPEND_A(c, listenA, NULL);
    struct elementA *e = APPEND_A(c, acceptA, NULL);
                         APPEND_A(c, _newconnA, NULL);
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
