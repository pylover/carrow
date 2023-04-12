#ifndef TCP_H
#define TCP_H


#include <sys/types.h>
#include <sys/socket.h>


enum tcpconnstatus {
    TCSIDLE,
    TCSCONNECTING,
    TCSCONNECTED,
    TCSFAILED,
};


struct tcpconn {
    enum tcpconnstatus status;

    int fd;
    struct sockaddr localaddr;
    struct sockaddr remoteaddr;
};


int
tcp_connect(struct tcpconn *c, const char *hostname, const char*port);


#endif
