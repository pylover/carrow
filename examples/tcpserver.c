#include "tty.h"
#include "carrow.h"

#include "addr.h"

#include <mrb.h>
#include <clog.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>


/* TCP server carrow types and function */
typedef struct tcpserver {
    const char *bindaddr;
    unsigned short bindport;
    int backlog;
} tcpserver;


#undef CARROW_ENTITY
#define CARROW_ENTITY tcpserver
#include "carrow_generic.h"
#include "carrow_generic.c"


/* TCP connection carrow types and function */
typedef struct conn {
    int fd;
    struct sockaddr localaddr;
    struct sockaddr remoteaddr;
    mrb_t buff;
} conn;


#undef CARROW_ENTITY
#define CARROW_ENTITY conn
#include "carrow_generic.h"
#include "carrow_generic.c"


#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


#define WORKING 99999999
volatile int status = WORKING;
static struct sigaction old_action;


void
echoA(struct conn_coro *self, struct conn *conn) {
    ssize_t bytes;
    struct mrb *buff = conn->buff;
    CORO_START;
    DEBUG("New conn: %s", sockaddr_dump(&conn->remoteaddr));

    while (true) {
        if (mrb_isfull(buff)) {
            CORO_REJECT("connection: %d buff full", conn->fd);
        }

        /* tcp read */
        CORO_WAIT(conn->fd, CIN);
        bytes = mrb_readin(buff, conn->fd, mrb_available(buff));
        if (bytes <= 0) {
            CORO_REJECT("read(%d)", conn->fd);
        }

        /* tcp write */
        if (mrb_isempty(buff)) {
            continue;
        }
        
        CORO_WAIT(conn->fd, COUT);
        bytes = mrb_writeout(buff, conn->fd, mrb_used(buff));
        if (bytes <= 0) {
            CORO_REJECT("write(%d)", conn->fd);
        }

        /* reset errno and rewait events*/
        errno = 0;
    }

    CORO_FINALLY;
    if (conn->fd != -1) {
        conn_evloop_unregister(conn->fd);
        close(conn->fd);
    }
    if (mrb_destroy(conn->buff)) {
        ERROR("Cannot dispose buffers.");
    }
    free(conn);
    CORO_END;
}


void
listenA(struct tcpserver_coro *self, struct tcpserver *state) {
    socklen_t addrlen = sizeof(struct sockaddr);
    struct sockaddr bindaddr;
    struct sockaddr connaddr;
    static int fd;
    int connfd;
    int res;
    int option = 1;
    CORO_START;
    
    /* Parse listen address */
    sockaddr_parse(&bindaddr, state->bindaddr, state->bindport);
    
    /* Create socket */
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    /* Allow reuse the address */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Bind to tcp port */
    res = bind(fd, &bindaddr, sizeof(bindaddr)); 
    if (res) {
        CORO_REJECT("Cannot bind on: %s", sockaddr_dump(&bindaddr));
    }

    /* Listen */
    res = listen(fd, state->backlog); 
    INFO("Listening on: %s", sockaddr_dump(&bindaddr));
    if (res) {
        CORO_REJECT("Cannot listen on: %s", sockaddr_dump(&bindaddr));
    }
  
    while (true) {
        CORO_WAIT(fd, CIN);
        if (self->events == 0) {
            /* Terminating */
            CORO_REJECT("Terminating");
        }

        connfd = accept4(fd, &connaddr, &addrlen, SOCK_NONBLOCK);
        if ((connfd == -1) && (!CMUSTWAIT())) {
            CORO_REJECT("accept4");
        }

        /* New Connection */
        struct conn *c = malloc(sizeof(struct conn));
        if (c == NULL) {
            CORO_REJECT("Out of memory");
        }

        c->fd = connfd;
        c->localaddr = bindaddr;
        c->remoteaddr = connaddr;
        c->buff = mrb_create(BUFFSIZE);
        conn_coro_create_and_run(echoA, c);
    }

    CORO_FINALLY;
    if (errno) {
        close(fd);
        tcpserver_evloop_unregister(fd);
    }
    CORO_END;
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
    clog_verbosity = CLOG_DEBUG;

    /* Signal */
    catch_signal();

    struct tcpserver state = {
        .bindaddr = "0.0.0.0",
        .bindport = 3030,
        .backlog = 2,
    };
    
    return tcpserver_forever(listenA, &state, &status);
}
