#include "tcpc.h"
#include "carrow.c"

#include <stdlib.h>
#include <unistd.h>


struct tcpcc 
errorA(struct tcpcc *self, struct tcpc *conn, int no) {
    carrow_dearm(STDIN_FILENO);
    return tcpc_stop();
}


struct tcpcc 
stdioA(struct tcpcc *self, struct tcpc *conn) {
    ssize_t bytes;
    char tmp[1024];
    static struct event ev = {
        .fd = STDIN_FILENO,
        .op = EVIN,
    };
    
    DEBUG("stdio");
    if (tcpc_arm(self, conn, &ev)) {
        return REJECT(self, conn, "tcpc_arm(%d)", STDIN_FILENO);
    }

    if ((ev.fd == STDIN_FILENO) && (ev.op & EVIN)) {
        bytes = read(ev.fd, tmp, 1024);
        if (bytes == -1) {
            return REJECT(self, conn, "read(%d)", ev.fd);
        }

        write(STDOUT_FILENO, tmp, bytes);
        
        if ((bytes == 0) || (bytes == 1 && tmp[0] == 4)) {
            return REJECT(self, conn, "read(%d) EOF", ev.fd);
        }
        
    }
    return tcpc_stop();
}


int
main() {
    int ret = EXIT_SUCCESS;
    // tty_cannonical();
    clog_verbosity = CLOG_DEBUG;
    struct tcpc conn = {
        .host = "0.0.0.0",
        .port = "3030",
    };
    carrow_evloop_init();
    
    if (tcpc_runloop(stdioA, errorA, &conn, NULL)) {
        ret = EXIT_FAILURE;
    }
    carrow_evloop_deinit();
    // tty_restore();
    return ret;
}
