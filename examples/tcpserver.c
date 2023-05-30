#include "tcp.h"
#include "tty.h"
#include "carrow.h"

#include <mrb.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


/* TCP server carrow types and function */
typedef struct tcps {
    int listenfd;

    const char *bindaddr;
    unsigned short bindport;
} tcps;


#undef CARROW_ENTITY
#define CARROW_ENTITY tcps
#include "carrow_generic.h"
#include "carrow_generic.c"


/* TCP connection carrow types and function */
typedef struct tcpsc {
    struct tcpconn conn;
    mrb_t buff;
} tcpsc;


#undef CARROW_ENTITY
#define CARROW_ENTITY tcpsc
#include "carrow_generic.h"
#include "carrow_generic.c"


#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;


/*
                read            write
read ok         wait(rw)
write ok                        wait(rw)
eof             close           close
error           err             err
buffer full     wait(w)          
buffer empty                    wait(r)
*/


struct tcpsc_coro
connerrorA(struct tcpsc_coro *self, struct tcpsc *state, int no) {
    struct tcpconn *conn = &(state->conn);
    if (conn->fd != -1) {
        tcpsc_evloop_unregister(conn->fd);
        close(conn->fd);
    }
    if (mrb_destroy(state->buff)) {
        ERROR("Cannot dispose buffers.");
    }

    free(state);
    return tcpsc_coro_stop();
}


struct tcpsc_coro 
echoA(struct tcpsc_coro *self, struct tcpsc *state, int fd, int events) {
    ssize_t bytes;
    struct mrb *buff = state->buff;
    struct tcpconn *conn = &(state->conn);
    int avail = mrb_available(buff);
    int used = mrb_used(buff);

    /* tcp read */
    bytes = mrb_readin(buff, conn->fd, avail);
    if ((bytes <= 0) && (!CMUSTWAIT())) {
        return tcpsc_coro_reject(self, state, CDBG, "read(%d)", conn->fd);
    }
    avail -= bytes;
    used += bytes;

    /* tcp write */
    bytes = mrb_writeout(buff, conn->fd, used);
    if ((bytes <= 0) && (!CMUSTWAIT())) {
        return tcpsc_coro_reject(self, state, CDBG, "writeout(%d)", conn->fd);
    }
    used -= bytes;
    avail += bytes;

    /* reset errno and rewait events*/
    errno = 0;
    int op;

    /* tcp socket */
    op = CONCE | CET;
    if (avail) {
        op |= CIN;
    }

    if (used) {
        op |= COUT;
    }

    if (tcpsc_evloop_modify_or_register(self, state, conn->fd, op)) {
        return tcpsc_coro_reject(self, state, CDBG, "wait(%d)", fd);
    }

    return tcpsc_coro_stop();
}


struct tcps_coro 
errorA(struct tcps_coro *self, struct tcps *state, int no) {
    tcps_evloop_unregister(state->listenfd);
    close(state->listenfd);
    return tcps_coro_stop();
}


struct tcps_coro
acceptA(struct tcps_coro *self, struct tcps *state, int fd, int events) {
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr addr;
    
    if (events == 0) {
        /* Terminating */
        return tcps_coro_reject(self, state, CDBG, "terminating");
    }

    fd = accept4(state->listenfd, &addr, &addrlen, SOCK_NONBLOCK);
    if (fd == -1) {
        if (CMUSTWAIT()) {
            errno = 0;
            if (tcps_evloop_modify_or_register(self, state, 
                        state->listenfd, CIN | CET)) {
                return tcps_coro_reject(self, state, CDBG, "tcps_wait");
            }
            return tcps_coro_stop();
        }
        return tcps_coro_reject(self, state, CDBG, "accept4");
    }

    struct tcpsc *c = malloc(sizeof(struct tcpsc));
    if (c == NULL) {
        return tcps_coro_reject(self, state, CDBG, "Out of memory");
    }

    c->conn.fd = fd;
    c->conn.remoteaddr = addr;
    c->buff = mrb_create(BUFFSIZE);
    static struct tcpsc_coro echo = {echoA, connerrorA};
    if (tcpsc_evloop_register(&echo, c, c->conn.fd, CIN | COUT | CONCE)) {
        free(c);
        return tcps_coro_reject(self, state, CDBG, "tcpsc_wait");
    }
}


struct tcps_coro
listenA(struct tcps_coro *self, struct tcps *state, int fd, int events) {

    fd = tcp_listen(state->bindaddr, state->bindport);
    if (fd < 0) {
        goto failed;
    }
    
    state->listenfd = fd;
    return tcps_coro_create_from(self, acceptA);

failed:
    return tcps_coro_reject(self, state, CDBG, "tcp_listen(%s:%u)", 
            state->bindaddr, state->bindport);
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

    struct tcps state = {
        .bindaddr = "0.0.0.0",
        .bindport = 3030,
        .listenfd = -1,
    };
    
    tcps_forever(listenA, errorA, &state, &status);
    return ret;
}
