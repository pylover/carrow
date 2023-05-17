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
static struct carrow_event ev = {
    .fd = STDIN_FILENO,
    .op = EVIN,
};


/*
                read            write
read ok         wait(rw)
write ok                        wait(rw)
eof             close           close
error           err             err
buffer full     wait(w)          
buffer empty                    wait(r)
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
            goto failed;
        }
        res += bytes;
        count -= bytes;
    }

    return res;

failed: 
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
    int bytes = 0;

    if (ev.fd != fd || !(ev.op & EVIN)) {
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
    if (bytes == 0) {
        return -1;
    }

    if ((bytes == -1) && (!EVMUSTWAIT())) {
        return -1;
    }

    return res;
}


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
echoA(struct tcpsc_coro *self, struct tcpsc *state) {
    ssize_t bytes;
    struct mrb *buff = state->buff;
    struct tcpconn *conn = &(state->conn);
    int avail = mrb_available(buff);
    int used = mrb_used(buff);

    /* tcp read */
    bytes = readA(buff, conn->fd, avail);
    if (bytes == -1) {
        return tcpsc_coro_reject(self, state, __DBG__, "read(%d)", conn->fd);
    }
    avail -= bytes;
    used += bytes;

    /* tcp write */
    bytes = writeA(buff, conn->fd, used);
    if (bytes == -1) {
        return tcpsc_coro_reject(self, state, __DBG__, "writeead(%d)", 
                conn->fd);
    }
    used -= bytes;
    avail += bytes;

    /* reset errno and rewait events*/
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

    if (tcpsc_evloop_register(self, state, &ev, conn->fd, op)) {
        return tcpsc_coro_reject(self, state, __DBG__, "wait(%d)", ev.fd);
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
acceptA(struct tcps_coro *self, struct tcps *state) {
    int fd;
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr addr;

    fd = accept4(state->listenfd, &addr, &addrlen, SOCK_NONBLOCK);
    if (fd == -1) {
        if (EVMUSTWAIT()) {
            errno = 0;
            if (tcps_evloop_register(self, state, &ev, state->listenfd, 
                        EVIN | EVET)) {
                return tcps_coro_reject(self, state, __DBG__, "tcps_wait");
            }
            return tcps_coro_stop();
        }
        return tcps_coro_reject(self, state, __DBG__, "accept4");
    }

    struct tcpsc *c = malloc(sizeof(struct tcpsc));
    if (c == NULL) {
        return tcps_coro_reject(self, state, __DBG__, "Out of memory");
    }

    c->conn.fd = fd;
    c->conn.remoteaddr = addr;
    c->buff = mrb_create(BUFFSIZE);
    static struct tcpsc_coro echo = {echoA, connerrorA};
    if (tcpsc_evloop_register(&echo, c, &ev, c->conn.fd, 
                EVIN | EVOUT | EVONESHOT)) {
        free(c);
        return tcps_coro_reject(self, state, __DBG__, "tcpsc_wait");
    }
}


struct tcps_coro
listenA(struct tcps_coro *self, struct tcps *state) {
    int fd;

    fd = tcp_listen(state->bindaddr, state->bindport);
    if (fd < 0) {
        goto failed;
    }
    
    state->listenfd = fd;
    return tcps_coro_create_from(self, acceptA);

failed:
    return tcps_coro_reject(self, state, __DBG__, "tcp_listen(%s:%u)", 
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
    
    carrow_init();
    tcps_coro_create_and_run(listenA, errorA, &state);
    if (carrow_evloop(&status)) {
        ret = EXIT_FAILURE;
    }

    carrow_deinit();
    return ret;
}
