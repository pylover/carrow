#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H


enum tcpclientstatus {
    TCSIDLE,
    TCSCONNECTING,
    TCSCONNECTED,
    TCSFAILED,
};


struct tcpconn {
    const char *host;
    const char *port;
    enum tcpclientstatus status;

    struct sockaddr localaddr;
    struct sockaddr remoteaddr;
};


#define CSTATE  tcpconn
#define CCORO  tcpc
#define CNAME(n) tcpc_ ## n


#include "carrow.c"
#include "evloop.h"


struct tcpc 
errorA(struct tcpc *self, struct tcpconn *conn, int fd, int no) {
    ERROR("ERROR");
    carrow_dearm(fd);
    return tcpc_stop();
}


struct tcpc 
stdioA(struct tcpc *self, struct tcpconn *conn, int fd, int op) {
    int bytes;
    char tmp[1024];

    if (tcpc_arm(self, conn, STDIN_FILENO, EVIN)) {
        return REJECT(self, conn, fd, "tcpc_arm(%d)", STDIN_FILENO);
    }

    if ((fd == STDIN_FILENO) && (op & EVIN)) {
        bytes = read(fd, tmp, 1024);
        if (bytes == -1) {
            return REJECT(self, conn, fd, "read(%d)", fd);
        }

        if (bytes == 0) {
            return REJECT(self, conn, fd, "read(%d) EOF", fd);
        }
        
        write(STDOUT_FILENO, tmp, bytes);
    }
    return tcpc_stop();
}


int
main() {
    int ret = EXIT_SUCCESS;
    clog_verbosity = CLOG_DEBUG;
    struct tcpconn conn = {
        .host = "0.0.0.0",
        .port = "3030",
    };
    carrow_evloop_init();
    
    if (tcpc_runloop(stdioA, errorA, &conn, NULL)) {
        ret = EXIT_FAILURE;
    }
    carrow_evloop_deinit();
    return ret;
}
