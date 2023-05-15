#ifndef UNIX_H
#define UNIX_H


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


enum unixconnstatus {
    TCSIDLE,
    TCSCONNECTING,
    TCSCONNECTED,
    TCSFAILED,
};


struct unixconn {
    enum unixconnstatus status;

    int fd;
    struct sockaddr_un localaddr;
};


int
unix_listen(const char *sockfile);


#endif
