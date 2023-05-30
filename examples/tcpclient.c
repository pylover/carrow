#include "tcp.h"
#include "tty.h"
#include "carrow.h"

#include <clog.h>
#include <mrb.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


typedef struct tcpc {
    const char *hostname;
    const char *port;

    struct tcpconn conn;
    mrb_t inbuff;
    mrb_t outbuff;
} tcpc;


#undef CARROW_ENTITY
#define CARROW_ENTITY tcpc
#include "carrow_generic.h"
#include "carrow_generic.c"


#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;


struct tcpc_coro
errorA(struct tcpc_coro *self, struct tcpc *state, int no) {
    struct tcpconn *conn = &(state->conn);
    
    tcpc_evloop_unregister(STDIN_FILENO);
    tcpc_evloop_unregister(STDOUT_FILENO);
    if (conn->fd != -1) {
        tcpc_evloop_unregister(conn->fd);
    }
    
    if (conn->fd > 2) {
        close(conn->fd);
    }

    errno = 0;
    return tcpc_coro_stop();
}


struct tcpc_coro 
ioA(struct tcpc_coro *self, struct tcpc *state, int fd, int events) {
    ssize_t bytes;
    struct mrb *in = state->inbuff;
    struct mrb *out = state->outbuff;
    struct tcpconn *conn = &(state->conn);
    int outavail = mrb_available(out);
    int outused = mrb_used(out);
    int inavail = mrb_available(out);
    int inused = mrb_used(in);

    if ((events & CERR) || (events & CRDHUP)) {
        return tcpc_coro_reject(self, state, CDBG, "evloop", fd);
    }

    /* stdin read */
    if ((fd == STDIN_FILENO) && outavail) {
        bytes = mrb_readin(out, STDIN_FILENO, outavail);
        if (bytes == -1) {
            return tcpc_coro_reject(self, state, CDBG, "read(%d)", fd);
        }
        outavail -= bytes;
        outused += bytes;
    }

    /* stdout write */
    if ((fd == STDOUT_FILENO) && inused) {
        bytes = mrb_writeout(in, STDOUT_FILENO, inused);
        if (bytes == -1) {
            return tcpc_coro_reject(self, state, CDBG, "write(%d)", fd);
        }
        inused -= bytes;
        inavail += bytes;
    }

    /* tcp write */
    if ((fd == conn->fd) && outused) {
        bytes = mrb_writeout(out, conn->fd, outused);
        if ((bytes <= 0) && (!CMUSTWAIT())) {
            return tcpc_coro_reject(self, state, CDBG, "write(%d)", fd);
        }
        outused -= bytes;
        outavail += bytes;
    }

    if ((fd == conn->fd) && inavail) {
        /* tcp read */
        bytes = mrb_readin(in, conn->fd, inavail);
        if ((bytes <= 0) && (!CMUSTWAIT())) {
            return tcpc_coro_reject(self, state, CDBG, "read(%d)", conn->fd);
        }
        inavail -= bytes;
        inused += bytes;
    }

    /* reset errno and rearm events */
    errno = 0;
    int op;

    /* stdin */
    if (outavail && tcpc_evloop_modify_or_register(self, state, STDIN_FILENO, 
                CIN | CONCE | CET)) {
        return tcpc_coro_reject(self, state, CDBG, "wait(%d)", STDIN_FILENO);
    }

    /* stdout */
    if (inused && tcpc_evloop_modify_or_register(self, state, STDOUT_FILENO, 
                COUT | CONCE | CET)) {
        return tcpc_coro_reject(self, state, CDBG, "wait(%d)", STDOUT_FILENO);
    }

    /* tcp socket */
    if (outused || inavail) {
        op = CONCE | CET;
        if (inavail) {
            op |= CIN;
        }

        if (outused) {
            op |= COUT;
        }

        if (tcpc_evloop_modify_or_register(self, state, conn->fd, op)) {
            return tcpc_coro_reject(self, state, CDBG, "wait(%d)", fd);
        }
    }
    
    return tcpc_coro_stop();
}


struct tcpc_coro 
connectA(struct tcpc_coro *self, struct tcpc *state, int fd, int events) {
    struct tcpconn *conn = &(state->conn);
    if (tcp_connect(conn, state->hostname, state->port)) {
        goto failed;
    }

    if (errno == EINPROGRESS) {
        errno = 0;
        if (tcpc_evloop_register(self, state, conn->fd, COUT | CONCE)) {
            goto failed;
        }

        return tcpc_coro_stop();
    }

    return tcpc_coro_create_from(self, ioA);

failed:
    return tcpc_coro_reject(self, state, CDBG, "tcp_connect(%s:%s)", 
            state->hostname, state->port);
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

    /* Non blocking starndard input/output */
    if (stdin_nonblock() || stdout_nonblock()) {
        return EXIT_FAILURE;
    }

    struct tcpc state = {
        .hostname = "localhost",
        .port = "3030",
    };
    
    state.inbuff = mrb_create(BUFFSIZE);
    state.outbuff = mrb_create(BUFFSIZE);
    if (state.inbuff == NULL || state.outbuff == NULL) {
        ERROR("Cannot initialized buffers.");
        return EXIT_FAILURE;
    }

    ret = tcpc_forever(connectA, errorA, &state, &status);
    if (mrb_destroy(state.inbuff) || mrb_destroy(state.outbuff)) {
        ERROR("Cannot dispose buffers.");
        ret = EXIT_FAILURE;
    }
    return ret;
}
