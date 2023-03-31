#include "carrow.h"
#include "tcp.h"
#include "addr.h"

#include <clog.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>


struct connstate {
    int fd;
    struct sockaddr remoteaddr;
};


void 
_conn_error(struct circuitA *c, struct connstate *state, const char *msg) {
    ERROR("Connection error: %s", msg);
    
    close(state->fd);
    free(state);
    freeA(c);
}


#define CHUNK   1024
static char tmp[CHUNK];
static ssize_t rbytes;
static ssize_t wbytes;


struct elementA *
_echoA(struct circuitA *c, struct connstate *s) {
    int fd = s->fd;
    int want = 0;
    
    if (rbytes == 0) {
        rbytes = read(fd, tmp, CHUNK);
        if (rbytes < 0) {
            if (EVMUSTWAIT()) {
                errno = 0;
                want |= EVIN;
            }
            else {
                return errorA(c, s, "read error");
            }
        }

        if (rbytes == 0) {
            return errorA(c, s, "read EOF");
        }
    }

    if (rbytes > 0) {
        wbytes = write(fd, tmp, rbytes);
        if (wbytes < 0) {
            if (EVMUSTWAIT()) {
                errno = 0;
                want |= EVOUT;
            }
            else {
                return errorA(c, s, "write error");
            }
        }

        if (wbytes == 0) {
            return errorA(c, s, "write EOF");
        }

        if (wbytes != rbytes) {
            return errorA(c, s, "read write size mismatch");
        }
    }

    rbytes = 0;
    wbytes = 0;
    if (want) {
        return evwaitA(c, (struct evstate*)s, fd, want);
    }

    // DEBUG("fd: %d, r: %ld, w: %ld", fd, rbytes, wbytes);
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
