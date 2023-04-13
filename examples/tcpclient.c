#include "tcp.h"
#include "tty.h"

#include <mrb.h>

#include <stdlib.h>
#include <unistd.h>


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


struct tcpcc 
errorA(struct tcpcc *self, struct state *state, int no) {
    struct tcpconn *conn = &(state->conn);

    if (conn->fd != -1) {
        tcpc_dearm(conn->fd);
    }
    close(conn->fd);
    return tcpc_stop();
}


struct tcpcc 
ioA(struct tcpcc *self, struct state *state) {
    ssize_t bytes;
    size_t avail = 0;
    struct mrb *in = state->inbuff;
    struct mrb *out = state->outbuff;
    struct tcpconn *conn = &(state->conn);
   
    static struct event ev = {
        .fd = STDIN_FILENO,
        .op = EVIN,
    };

    if (ev.fd == STDIN_FILENO) {
        avail = mrb_available(out);
        while (avail) {
            bytes = mrb_readin(out, STDIN_FILENO, avail);
            if (bytes <= 0) {
                break;
            }
            avail -= bytes;
        }
       
        if (bytes == -1) {
            if (!EVMUSTWAIT()) {
                return REJECT(self, state, "read(%d)", ev.fd);
            }
        }
    }

    if (ev.fd == STDOUT_FILENO) {
        avail = mrb_used(in);
        while (avail) {
            bytes = mrb_writeout(in, STDOUT_FILENO, avail);
            if (bytes <= 0) {
                break;
            }
            avail -= bytes;
        }
       
        if (bytes == -1) {
            if (!EVMUSTWAIT()) {
                return REJECT(self, state, "read(%d)", ev.fd);
            }
        }
    }

    if ((ev.fd == conn->fd) && (ev.op & EVOUT)) {
        /* Writing to TCP socket */
        avail = mrb_used(out);
        while (avail) {
            bytes = mrb_writeout(out, conn->fd, avail);
            if (bytes <= 0) {
                break;
            }
            avail -= bytes;
        }
       
        if (bytes == -1) {
            if (!EVMUSTWAIT()) {
                return REJECT(self, state, "read(%d)", ev.fd);
            }
        }
    }
    
    if ((ev.fd == conn->fd) && (ev.op & EVIN)) {
        /* Reading from TCP socket */
        avail = mrb_available(in);
        while (avail) {
            bytes = mrb_readin(in, conn->fd, avail);
            if (bytes <= 0) {
                break;
            }
            avail -= bytes;
        }
       
        if (bytes == -1) {
            if (!EVMUSTWAIT()) {
                return REJECT(self, state, "read(%d)", conn->fd);
            }
        }
    }
   
    errno = 0;
    size_t inavail = mrb_available(in);
    size_t outavail = mrb_available(out);
    size_t inused = mrb_used(in);
    size_t outused = mrb_used(out);

    /* stdin */
    if (outavail) {
        ev.fd = STDIN_FILENO;
        // ev.op = EVIN | EVONESHOT;
        ev.op = EVIN;
        if (tcpc_arm(self, state, &ev)) {
            return REJECT(self, state, "arm(%d)", ev.fd);
        }
    }

    /* stdout */
    if (inused) {
        ev.fd = STDOUT_FILENO;
        ev.op = EVOUT | EVONESHOT;
        if (tcpc_arm(self, state, &ev)) {
            return REJECT(self, state, "arm(%d)", ev.fd);
        }
    }

    /* tcp socket */
    if (outused || inavail) {
        ev.fd = conn->fd;
        ev.op = EVONESHOT;
        if (inavail) {
            ev.op = EVIN;
        }

        if (outused) {
            ev.op = EVOUT;
        }

        if (tcpc_arm(self, state, &ev)) {
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

    ev.fd = conn->fd;
    ev.op = EVOUT | EVONESHOT;
    if (errno == EINPROGRESS) {
        errno = 0;
        if (tcpc_arm(self, state, &ev)) {
            goto failed;
        }

        return tcpc_stop();
    }

    return tcpc_from(self, ioA);

failed:
    return REJECT(self, state, "tcp_connect(%s:%s)", state->hostname, 
            state->port);
}


int
main() {
    int ret = EXIT_SUCCESS;
    if (stdin_nonblock() || stdout_nonblock()) {
        return EXIT_FAILURE;
    }

    clog_verbosity = CLOG_DEBUG;
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
    
    if (tcpc_runloop(connectA, errorA, &state, NULL)) {
        ret = EXIT_FAILURE;
    }

    carrow_evloop_deinit();
    if (mrb_destroy(state.inbuff) || mrb_destroy(state.outbuff)) {
        ERROR("Cannot dispose buffers.");
        ret = EXIT_FAILURE;
    }
    return ret;
}
