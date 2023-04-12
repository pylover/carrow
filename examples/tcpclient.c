#include "tcp.h"
#include "tty.h"

#include <stdlib.h>
#include <unistd.h>


struct state {
    const char *hostname;
    const char *port;

    struct tcpconn conn;
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
    DEBUG("IO A");
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


// struct tcpcc 
// stdioA(struct tcpcc *self, struct state *state) {
//     ssize_t bytes;
//     char tmp[1024];
//     
//     if (tcpc_arm(self, conn, &ev)) {
//         return REJECT(self, conn, "tcpc_arm(%d)", STDIN_FILENO);
//     }
// 
//     if ((ev.fd == STDIN_FILENO) && (ev.op & EVIN)) {
//         bytes = read(ev.fd, tmp, 1024);
//         if (bytes == -1) {
//             return REJECT(self, conn, "read(%d)", ev.fd);
//         }
// 
//         write(STDOUT_FILENO, tmp, bytes);
//         
//         if ((bytes == 0) || (bytes == 1 && tmp[0] == 4)) {
//             return REJECT(self, conn, "read(%d) EOF", ev.fd);
//         }
//         
//     }
//     return tcpc_stop();
// }


int
main() {
    int ret = EXIT_SUCCESS;
    if (isatty(STDIN_FILENO) && stdin_noncannonical()) {
        return EXIT_FAILURE;
    }

    clog_verbosity = CLOG_DEBUG;
    struct state state = {
        .hostname = "localhost",
        .port = "3030",
    };
    carrow_evloop_init();
    
    if (tcpc_runloop(connectA, errorA, &state, NULL)) {
        ret = EXIT_FAILURE;
    }
    carrow_evloop_deinit();
    if (stdin_restore()) {
        return EXIT_FAILURE;
    }
    return ret;
}
