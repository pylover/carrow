#include "tcp.h"
#include "tty.h"

#include <mrb.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


/* TCP server carrow types and function */
struct state {
    int listenfd;

    const char *bindaddr;
    unsigned short bindport;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   state
#define CCORO    tcpsc
#define CNAME(n) tcps_ ## n
#include "carrow.h"
#include "carrow.c"


/* TCP connection carrow types and function */
struct connstate {
    struct tcpconn conn;
    mrb_t buff;
    struct event ev;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   connstate
#define CCORO    tcpcc
#define CNAME(n) tcpcc_ ## n
#include "carrow.h"
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


ssize_t
writeA(struct mrb *b, int fd, size_t count) {
    int res = 0;
    int bytes;

    if (ev.fd != fd || !(ev.op & EVOUT)) {
        return 0;
    }

    while (count) {
        bytes = mrb_writeout(b, fd, count);
        if (bytes <= 0) {
            break;
        }
        res += bytes;
        count -= bytes;
    }
    
    if ((bytes <= 0) && (!EVMUSTWAIT())) {
        return -1;
    }

    return res;
}


ssize_t
readA(struct mrb *b, int fd, size_t count) {
    int res = 0;
    int bytes;

    if (ev.fd != fd || !(ev.op & EVIN)) {
        return 0;
    }

    while (count) {
        bytes = mrb_readin(b, fd, count);
        if (bytes <= 0) {
            break;
        }
        res += bytes;
        count -= bytes;
    }
    
    if ((bytes <= 0) && (!EVMUSTWAIT())) {
        return -1;
    }

    return res;
}


struct tcpcc 
connerrorA(struct tcpcc *self, struct connstate *state, int no) {
    struct tcpconn *conn = &(state->conn);
    if (conn->fd != -1) {
        tcpcc_dearm(conn->fd);
        close(conn->fd);
    }
    if (mrb_destroy(state->buff)) {
        ERROR("Cannot dispose buffers.");
    }

    DEBUG("connerrorA: %p", state);
    free(state);
    return tcpcc_stop();
}


struct tcpcc 
echoA(struct tcpcc *self, struct connstate *state) {
    ssize_t bytes;
    struct mrb *buff = state->buff;
    struct tcpconn *conn = &(state->conn);
    int avail = mrb_available(buff);
    int used = mrb_used(buff);

    /* tcp read */
    bytes = readA(buff, conn->fd, avail);
    DEBUG("read: %ld", bytes);
    if (bytes == -1) {
        return tcpcc_reject(self, state, DBG, "read(%d)", conn->fd);
    }
    avail -= bytes;
    used += bytes;

    /* tcp write */
    bytes = writeA(buff, conn->fd, used);
    DEBUG("write: %ld", bytes);
    if (bytes == -1) {
        return tcpcc_reject(self, state, DBG, "writeead(%d)", conn->fd);
    }
    used -= bytes;
    avail += bytes;

    /* reset errno and rearm events*/
    errno = 0;
    int op;

    /* tcp socket */
    op = EVONESHOT | EVET;
    if (avail) {
        op |= EVIN;
    }

    if (used) {
        op |= EVOUT;
    }

    if (tcpcc_arm(self, state, &ev, conn->fd, op)) {
        return REJECT(self, state, "arm(%d)", ev.fd);
    }

    DEBUG("Echo: %d", state->conn.fd);
    return tcpcc_stop();
}


struct tcpsc 
errorA(struct tcpsc *self, struct state *state, int no) {
    tcps_dearm(state->listenfd);
    close(state->listenfd);
    return tcps_stop();
}


struct tcpsc
acceptA(struct tcpsc *self, struct state *state) {
    DEBUG("New conn");
    int fd;
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr addr;

    fd = accept4(state->listenfd, &addr, &addrlen, SOCK_NONBLOCK);
    if (fd == -1) {
        if (EVMUSTWAIT()) {
            errno = 0;
            if (tcps_arm(self, state, &ev, state->listenfd, EVIN | EVET)) {
                return tcps_reject(self, state, DBG, "tcpcc_arm");
            }
            return tcps_stop();
        }
        return tcps_reject(self, state, DBG, "accept4");
    }

    struct connstate *c = malloc(sizeof(struct connstate));
    if (c == NULL) {
        return tcps_reject(self, state, DBG, "Out of memory");
    }

    c->conn.fd = fd;
    c->conn.remoteaddr = addr;
    c->buff = mrb_create(BUFFSIZE);
    static struct tcpcc echo = {echoA, connerrorA};
    if (tcpcc_arm(&echo, c, &(c->ev), c->conn.fd, 
                EVIN | EVOUT | EVONESHOT)) {
        free(c);
        return tcps_reject(self, state, DBG, "tcpcc_arm");
    }
}


struct tcpsc 
listenA(struct tcpsc *self, struct state *state) {
    int fd;

    fd = tcp_listen(state->bindaddr, state->bindport);
    if (fd < 0) {
        goto failed;
    }
    
    state->listenfd = fd;
    return tcps_from(self, acceptA);

failed:
    return tcps_reject(self, state, DBG, "tcp_listen(%s:%u)", 
            state->bindaddr, state->bindport);
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

    struct state state = {
        .bindaddr = "0.0.0.0",
        .bindport = 3030,
        .listenfd = -1,
    };
    
    carrow_evloop_init();
    if (tcps_runloop(listenA, errorA, &state, &status)) {
        ret = EXIT_FAILURE;
    }

    carrow_evloop_deinit();
    return ret;
}
