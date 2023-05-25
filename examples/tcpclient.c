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
errorA(struct tcpc_coro *self, struct tcpc *state, int fd, int events, 
        int no) {
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


ssize_t
writeA(struct mrb *b, int fd, size_t count, int events) {
    int res = 0;
    int bytes = 0;

    if (!(events & EVOUT)) {
        return 0;
    }

    while (count) {
        bytes = mrb_writeout(b, fd, count);
        if (bytes <= 0) {
            goto failed;
        }
        res += bytes;
        count -= bytes;
    }

    return res;
   
failed:
    if ((bytes == -1) && (!EVMUSTWAIT())) {
        return -1;
    }

    return res;
}


ssize_t
readA(struct mrb *b, int fd, size_t count, int events) {
    int res = 0;
    int bytes = 0;

    if (!(events & EVIN)) {
        return 0;
    }

    while (count) {
        bytes = mrb_readin(b, fd, count);
        if (bytes <= 0) {
            goto failed;
        }
        res += bytes;
        count -= bytes;
    }
    
    return res;

failed:
    if ((bytes == -1) && (!EVMUSTWAIT())) {
        return -1;
    }

    return res;
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

    DEBUG("events: %d", events);
    if ((events & EVERR) || (events & EVRDHUP)) {
        return tcpc_coro_reject(self, state, fd, events, __DBG__, "evloop", 
                fd);
    }

    /* stdin read */
    bytes = readA(out, STDIN_FILENO, outavail, events);
    if (bytes == -1) {
        return tcpc_coro_reject(self, state, fd, events, __DBG__, "read(%d)", 
                fd);
    }
    outavail -= bytes;
    outused += bytes;

    /* stdout write */
    bytes = writeA(in, STDOUT_FILENO, inused, events);
    if (bytes == -1) {
        return tcpc_coro_reject(self, state, fd, events, __DBG__, "write(%d)", 
                fd);
    }
    inused -= bytes;
    inavail += bytes;

    /* tcp write */
    bytes = writeA(out, conn->fd, outused, events);
    if (bytes == -1) {
        return tcpc_coro_reject(self, state, fd, events, __DBG__, "write(%d)", 
                fd);
    }
    outused -= bytes;
    outavail += bytes;

    /* tcp read */
    bytes = readA(in, conn->fd, inavail, events);
    if (bytes == -1) {
        return tcpc_coro_reject(self, state, fd, events, __DBG__, "read(%d)", 
                conn->fd);
    }
    inavail -= bytes;
    inused += bytes;

    /* reset errno and rearm events */
    errno = 0;
    int op;

    /* stdin */
    if (outavail && tcpc_evloop_modify_or_register(self, state, STDIN_FILENO, 
                EVIN | EVONESHOT | EVET)) {
        return tcpc_coro_reject(self, state, fd, events, __DBG__, "wait(%d)", 
                STDIN_FILENO);
    }

    /* stdout */
    if (inused && tcpc_evloop_modify_or_register(self, state, STDOUT_FILENO, 
                EVOUT | EVONESHOT | EVET)) {
        return tcpc_coro_reject(self, state, fd, events, __DBG__, "wait(%d)", 
                STDOUT_FILENO);
    }

    /* tcp socket */
    if (outused || inavail) {
        op = EVONESHOT | EVET;
        if (inavail) {
            op |= EVIN;
        }

        if (outused) {
            op |= EVOUT;
        }

        if (tcpc_evloop_modify_or_register(self, state, conn->fd, op)) {
            return tcpc_coro_reject(self, state, fd, events, __DBG__, 
                    "wait(%d)", fd);
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
        DEBUG("reg connect: %d", conn->fd);
        if (tcpc_evloop_register(self, state, conn->fd, EVOUT | EVONESHOT)) {
            goto failed;
        }

        return tcpc_coro_stop();
    }

    return tcpc_coro_create_from(self, ioA);

failed:
    return tcpc_coro_reject(self, state, fd, events, __DBG__, 
            "tcp_connect(%s:%s)", state->hostname, state->port);
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

    carrow_init();
   
    tcpc_coro_create_and_run(connectA, errorA, &state, -1, 0);
    if (carrow_evloop(&status)) {
        ret = EXIT_FAILURE;
    }

    carrow_deinit();
    if (mrb_destroy(state.inbuff) || mrb_destroy(state.outbuff)) {
        ERROR("Cannot dispose buffers.");
        ret = EXIT_FAILURE;
    }
    return ret;
}
