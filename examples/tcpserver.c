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
#define CCORO    tcps
#define CNAME(n) tcps_ ## n
#include "carrow.c"


/* TCP connection carrow types and function */
struct connstate {
    struct tcpconn conn;
    mrb_t buff;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   connstate
#define CCORO    tcpsc
#define CNAME(n) tcpsc_ ## n
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


/*
                read            write
read ok         arm(rw)
write ok                        arm(rw)
eof             close           close
error           err             err
buffer full     arm(w)          
buffer empty                    arm(r)
*/


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
   
    if (bytes == 0) {
        return -1;
    }

    if ((bytes == -1) && (!EVMUSTWAIT())) {
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
    
    if (bytes == 0) {
        return -1;
    }

    if ((bytes == -1) && (!EVMUSTWAIT())) {
        return -1;
    }

    return res;
}


struct tcpsc 
connerrorA(struct tcpsc *self, struct connstate *state, int no) {
    struct tcpconn *conn = &(state->conn);
    if (conn->fd != -1) {
        tcpsc_dearm(conn->fd);
        close(conn->fd);
    }
    if (mrb_destroy(state->buff)) {
        ERROR("Cannot dispose buffers.");
    }

    free(state);
    return tcpsc_stop();
}


struct tcpsc 
echoA(struct tcpsc *self, struct connstate *state) {
    ssize_t bytes;
    struct mrb *buff = state->buff;
    struct tcpconn *conn = &(state->conn);
    int avail = mrb_available(buff);
    int used = mrb_used(buff);

    /* tcp read */
    bytes = readA(buff, conn->fd, avail);
    if (bytes == -1) {
        return tcpsc_reject(self, state, DBG, "read(%d)", conn->fd);
    }
    avail -= bytes;
    used += bytes;

    /* tcp write */
    bytes = writeA(buff, conn->fd, used);
    if (bytes == -1) {
        return tcpsc_reject(self, state, DBG, "writeead(%d)", conn->fd);
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

    if (tcpsc_arm(self, state, &ev, conn->fd, op)) {
        return tcpsc_reject(self, state, DBG, "arm(%d)", ev.fd);
    }

    return tcpsc_stop();
}


struct tcps 
errorA(struct tcps *self, struct state *state, int no) {
    tcps_dearm(state->listenfd);
    close(state->listenfd);
    return tcps_stop();
}


struct tcps
acceptA(struct tcps *self, struct state *state) {
    int fd;
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr addr;

    fd = accept4(state->listenfd, &addr, &addrlen, SOCK_NONBLOCK);
    if (fd == -1) {
        if (EVMUSTWAIT()) {
            errno = 0;
            if (tcps_arm(self, state, &ev, state->listenfd, EVIN | EVET)) {
                return tcps_reject(self, state, DBG, "tcpsc_arm");
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
    static struct tcpsc echo = {echoA, connerrorA};
    if (tcpsc_arm(&echo, c, &ev, c->conn.fd, EVIN | EVOUT | EVONESHOT)) {
        free(c);
        return tcps_reject(self, state, DBG, "tcpsc_arm");
    }
}


struct tcps 
listenA(struct tcps *self, struct state *state) {
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
