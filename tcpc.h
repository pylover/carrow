#ifndef TCPC_H
#define TCPC_H


#include <sys/types.h>
#include <sys/socket.h>


enum tcpcstatus {
    TCSIDLE,
    TCSCONNECTING,
    TCSCONNECTED,
    TCSFAILED,
};


struct tcpc {
    const char *host;
    const char *port;
    enum tcpcstatus status;

    int fd;
    struct sockaddr localaddr;
    struct sockaddr remoteaddr;
};


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H


#define CSTATE  tcpc
#define CCORO   tcpcc
#define CNAME(n) tcpc_ ## n
#include "carrow.h"


#endif
