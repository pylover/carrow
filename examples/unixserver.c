#include "unix.h"
#include "tty.h"

#include <mrb.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


/* UNIX server carrow types and function */
struct state {
    int listenfd;

    const char *sockfile;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   struct state
#define CCORO    unixs
#define CNAME(n) unixs_ ## n
#include "carrow.h"
#include "carrow.c"


/* UNIX connection carrow types and function */
struct connstate {
    struct unixconn conn;
    mrb_t buff;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H

#define CSTATE   struct connstate
#define CCORO    unixsc
#define CNAME(n) unixsc_ ## n
#include "carrow.h"
#include "carrow.c"


#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;
static struct carrow_event ev = {
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


struct unixsc 
connerrorA(struct unixsc *self, struct connstate *state, int no) {
    struct unixconn *conn = &(state->conn);
    if (conn->fd != -1) {
        unixsc_nowait(conn->fd);
        close(conn->fd);
    }
    if (mrb_destroy(state->buff)) {
        ERROR("Cannot dispose buffers.");
    }

    free(state);
    return unixsc_stop();
}


struct unixsc 
echoA(struct unixsc *self, struct connstate *state) {
    ssize_t bytes;
    struct mrb *buff = state->buff;
    struct unixconn *conn = &(state->conn);
    int avail = mrb_available(buff);
    int used = mrb_used(buff);

    /* unix read */
    bytes = readA(buff, conn->fd, avail);
    if (bytes == -1) {
        return unixsc_reject(self, state, DBG, "read(%d)", conn->fd);
    }
    avail -= bytes;
    used += bytes;

    /* unix write */
    bytes = writeA(buff, conn->fd, used);
    if (bytes == -1) {
        return unixsc_reject(self, state, DBG, "writeead(%d)", conn->fd);
    }
    used -= bytes;
    avail += bytes;

    /* reset errno and rewait events*/
    errno = 0;
    int op;

    /* unix socket */
    op = EVONESHOT | EVET;
    if (avail) {
        op |= EVIN;
    }

    if (used) {
        op |= EVOUT;
    }

    if (unixsc_wait(self, state, &ev, conn->fd, op)) {
        return unixsc_reject(self, state, DBG, "wait(%d)", ev.fd);
    }

    return unixsc_stop();
}


struct unixs 
errorA(struct unixs *self, struct state *state, int no) {
    unixs_nowait(state->listenfd);
    close(state->listenfd);
    return unixs_stop();
}


struct unixs
acceptA(struct unixs *self, struct state *state) {
    int fd;
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr addr;

    fd = accept4(state->listenfd, &addr, &addrlen, SOCK_NONBLOCK);
    if (fd == -1) {
        if (EVMUSTWAIT()) {
            errno = 0;
            if (unixs_wait(self, state, &ev, state->listenfd, EVIN | EVET)) {
                return unixs_reject(self, state, DBG, "unixs_wait");
            }
            return unixs_stop();
        }
        return unixs_reject(self, state, DBG, "accept4");
    }

    struct connstate *c = malloc(sizeof(struct connstate));
    if (c == NULL) {
        return unixs_reject(self, state, DBG, "Out of memory");
    }

    c->conn.fd = fd;
    c->buff = mrb_create(BUFFSIZE);
    static struct unixsc echo = {echoA, connerrorA};
    if (unixsc_wait(&echo, c, &ev, c->conn.fd, EVIN | EVOUT | EVONESHOT)) {
        free(c);
        return unixs_reject(self, state, DBG, "unixsc_wait");
    }
}


struct unixs 
listenA(struct unixs *self, struct state *state) {
    int fd;

    fd = unix_listen(state->sockfile);
    if (fd < 0) {
        goto failed;
    }
    
    state->listenfd = fd;
    return unixs_from(self, acceptA);

failed:
    return unixs_reject(self, state, DBG, "unix_listen(%s)", state->sockfile);
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

    struct state state = {
        .sockfile = "foo",
        .listenfd = -1,
    };
    
    carrow_init();
    if (unixs_runloop(listenA, errorA, &state, &status)) {
        ret = EXIT_FAILURE;
    }

    carrow_deinit();
    return ret;
}
