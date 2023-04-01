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
#define CHUNK   32768
#define BUFFSIZE 327680


static volatile int status = WORKING;
static struct sigaction old_action;
static struct circuitA *conncircuit = NULL;


struct connstate {
    int fd;
    struct sockaddr remoteaddr;
    struct mrb buff;

    size_t bytesin;
    size_t bytesout;
};


void 
_conn_error(struct circuitA *c, struct connstate *state, const char *msg) {
    ERROR("Connection error: %s", msg);
    
    close(state->fd);
    evdearm(state->fd);
    mrb_deinit(&(state->buff));
    free(state);
}


struct elementA *
_echoA(struct circuitA *c, struct connstate *s) {
    int fd = s->fd;
    int want = 0;
    ssize_t bytes;
    struct mrb *b = &(s->buff);
    size_t toread = mrb_space_available(b);
    
    while (toread) {
        bytes = mrb_readin(b, fd, toread);
        if (bytes < 0) {
            if (EVMUSTWAIT()) {
                errno = 0;
                want |= EVIN;
                break;
            }
            else {
                return errorA(c, s, "read error");
            }
        }
        else if (bytes == 0) {
            return errorA(c, s, "read EOF");
        }
        else {
            s->bytesin += bytes;
            toread -= bytes;
        }
    }

    size_t towrite = mrb_space_used(b);
    while (towrite) {
        bytes = mrb_writeout(b, fd, towrite);
        if (bytes < 0) {
            if (EVMUSTWAIT()) {
                errno = 0;
                want |= EVOUT;
                break;
            }
            else {
                return errorA(c, s, "write error");
            }
        }
        else if (bytes == 0) {
            return errorA(c, s, "write EOF");
        }
        else {
            s->bytesout += bytes;
            towrite -= bytes;
        }
    }

    if (want) {
        DEBUG("Echo: in: %lu, out: %lu", s->bytesin, s->bytesout);
        return evwaitA(c, (struct evstate*)s, fd, want);
    }

    return nextA(c, s);
}


struct circuitA *
_get_conncircuit() {
    if (conncircuit == NULL) {
        conncircuit = NEW_A(_conn_error);
        struct elementA *e = APPEND_A(conncircuit, _echoA, NULL);
                   loopA(e); 
    }
    
    return conncircuit;
}


static struct elementA *
_newconnA(struct circuitA *c, struct tcpsrvstate *s) {
    INFO("New connection: %s", carrow_sockaddr_dump(&(s->newconnaddr)));

    struct circuitA *cc = _get_conncircuit();
    struct connstate *cs = malloc(sizeof (struct connstate));
    memcpy(&(cs->remoteaddr), &(s->newconnaddr), s->addrlen);
    cs->fd = s->newconnfd; 
    cs->bytesin = 0;
    cs->bytesout = 0;

    if (mrb_init(&(cs->buff), BUFFSIZE)) {
        return errorA(c, s, "mrb_init");
    }
    runA(cc, cs);


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
    if (evloop(&status)) {
        ret = EXIT_FAILURE;
    }
    close(state.listenfd);
    evdeinitA();
    freeA(conncircuit);
    freeA(c);
    return ret;
}
