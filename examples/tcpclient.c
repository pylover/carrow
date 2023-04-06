#include <stdlib.h>
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
}


struct tcpc 
stdioA(struct tcpc *self, struct tcpconn *conn, int fd, int op) {
}


int
main() {
    int ret = EXIT_SUCCESS;
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
