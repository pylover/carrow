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
    struct evstate;
    struct sockaddr remoteaddr;
    struct mrb buff;

    size_t bytesin;
    size_t bytesout;
};


struct elementA * 
_conn_error(struct circuitA *c, struct connstate *s, const char *msg) {
    ERROR("Connection error: %s", msg);
    
    DEBUG("Closed: %d in: %lu, out: %lu", s->fd, s->bytesin, s->bytesout);
    close(s->fd);
    evdearm(s->fd);
    mrb_deinit(&(s->buff));
    free(s);
    return disposeA(c);
}


struct elementA *
_echoA(struct circuitA *c, struct connstate *s) {
    int fd = s->fd;
    int events = s->events;
    int want = 0;
    ssize_t bytes;
    struct mrb *b = &(s->buff);
    
    bool r = events & EVIN;
    bool w = events & EVOUT;
    // DEBUG("Ev: r: %d w: %d", r, w);

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
        r = want & EVIN;
        w = want & EVOUT;
        DEBUG("Wait: r: %d w: %d, fd: %d in: %lu, out: %lu", r, w, fd, s->bytesin, s->bytesout);
        return evwaitA(c, (struct evstate*)s, fd, want | EVET | EVONESHOT);
    }

    DEBUG("Next: %d in: %lu, out: %lu", fd, s->bytesin, s->bytesout);
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
    cs->bytesin = 0;
    cs->bytesout = 0;
    cs->events = EVIN | EVOUT;
    DEBUG("Open: %d in: %lu, out: %lu", cs->fd, cs->bytesin, cs->bytesout);

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
    freeA(c);
    return ret;
}
