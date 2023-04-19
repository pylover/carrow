#include "tcp.h"
#include "tty.h"

#include <mrb.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


struct state {
    const char *hostname;
    const char *port;

    struct tcpconn conn;
    mrb_t inbuff;
    mrb_t outbuff;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   state
#define CCORO    tcpcc
#define CNAME(n) tcpc_ ## n
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



struct tcpcc 
errorA(struct tcpcc *self, struct state *state, int no) {
    struct tcpconn *conn = &(state->conn);
    
    tcpc_dearm(STDIN_FILENO);
    tcpc_dearm(STDOUT_FILENO);
    if (conn->fd != -1) {
        tcpc_dearm(conn->fd);
    }
    
    if (conn->fd > 2) {
        close(conn->fd);
    }

    errno = 0;
    return tcpc_stop();
}


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
    
    if ((bytes == -1) && (!EVMUSTWAIT())) {
        return -1;
    }

    return res;
}


struct tcpcc 
ioA(struct tcpcc *self, struct state *state) {
    ssize_t bytes;
    struct mrb *in = state->inbuff;
    struct mrb *out = state->outbuff;
    struct tcpconn *conn = &(state->conn);
    int outavail = mrb_available(out);
    int outused = mrb_used(out);
    int inavail = mrb_available(out);
    int inused = mrb_used(in);
 
    /* stdin read */
    bytes = readA(out, STDIN_FILENO, outavail);
    if (bytes == -1) {
        return REJECT(self, state, "read(%d)", ev.fd);
    }
    outavail -= bytes;
    outused += bytes;

    /* stdout write */
    bytes = writeA(in, STDOUT_FILENO, inused);
    if (bytes == -1) {
        return REJECT(self, state, "write(%d)", ev.fd);
    }
    inused -= bytes;
    inavail += bytes;

    /* tcp write */
    bytes = writeA(out, conn->fd, outused);
    if (bytes == -1) {
        return REJECT(self, state, "writeead(%d)", ev.fd);
    }
    outused -= bytes;
    outavail += bytes;

    /* tcp read */
    bytes = readA(in, conn->fd, inavail);
    if (bytes == -1) {
        return REJECT(self, state, "read(%d)", conn->fd);
    }
    inavail -= bytes;
    inused += bytes;

    /* reset errno and rearm events*/
    errno = 0;
    int op;

    /* stdin */
    if (outavail && tcpc_arm(self, state, &ev, STDIN_FILENO, 
                EVIN | EVONESHOT)) {
        return REJECT(self, state, "arm(%d)", ev.fd);
    }

    /* stdout */
    if (inused && tcpc_arm(self, state, &ev, STDOUT_FILENO, 
                EVOUT | EVONESHOT)) {
        return REJECT(self, state, "arm(%d)", ev.fd);
    }

    /* tcp socket */
    if (outused || inavail) {
        op = EVONESHOT;
        if (inavail) {
            op = EVIN;
        }

        if (outused) {
            op = EVOUT;
        }

        if (tcpc_arm(self, state, &ev, conn->fd, op)) {
            return REJECT(self, state, "arm(%d)", ev.fd);
        }
    }
    
    return tcpc_stop();
}


struct tcpcc 
connectA(struct tcpcc *self, struct state *state) {
    struct tcpconn *conn = &(state->conn);
    static struct event ev = {
        .fd = -1,
        .op = EVIN,
    };

    if (tcp_connect(conn, state->hostname, state->port)) {
        goto failed;
    }

    if (errno == EINPROGRESS) {
        errno = 0;
        if (tcpc_arm(self, state, &ev, conn->fd, EVOUT | EVONESHOT)) {
            goto failed;
        }

        return tcpc_stop();
    }

    return tcpc_from(self, ioA);

failed:
    return REJECT(self, state, "tcp_connect(%s:%s)", state->hostname, 
            state->port);
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

    /* Non blocking starndard input/output */
    if (stdin_nonblock() || stdout_nonblock()) {
        return EXIT_FAILURE;
    }

    struct state state = {
        .hostname = "localhost",
        .port = "3030",
    };
    
    state.inbuff = mrb_create(BUFFSIZE);
    state.outbuff = mrb_create(BUFFSIZE);
    if (state.inbuff == NULL || state.outbuff == NULL) {
        ERROR("Cannot initialized buffers.");
        return EXIT_FAILURE;
    }

    carrow_evloop_init();
    
    if (tcpc_runloop(connectA, errorA, &state, &status)) {
        ret = EXIT_FAILURE;
    }

    carrow_evloop_deinit();
    if (mrb_destroy(state.inbuff) || mrb_destroy(state.outbuff)) {
        ERROR("Cannot dispose buffers.");
        ret = EXIT_FAILURE;
    }
    return ret;
}
