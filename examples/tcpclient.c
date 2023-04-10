#include "tcpc.h"
#include "carrow.c"

#include <stdlib.h>
#include <unistd.h>


struct tcpcc 
errorA(struct tcpcc *self, struct tcpc *conn, int fd, int no) {
    carrow_dearm(fd);
    return tcpc_stop();
}


struct tcpcc 
stdioA(struct tcpcc *self, struct tcpc *conn, int fd, int op) {
    ssize_t bytes;
    char tmp[1024];
    
    DEBUG("stdio");
    if (tcpc_arm(self, conn, STDIN_FILENO, EVIN)) {
        return REJECT(self, conn, fd, "tcpc_arm(%d)", STDIN_FILENO);
    }

    if ((fd == STDIN_FILENO) && (op & EVIN)) {
        bytes = read(fd, tmp, 1024);
        if (bytes == -1) {
            return REJECT(self, conn, fd, "read(%d)", fd);
        }

        write(STDOUT_FILENO, tmp, bytes);
        
        if ((bytes == 0) || (bytes == 1 && tmp[0] == 4)) {
            return REJECT(self, conn, fd, "read(%d) EOF", fd);
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
