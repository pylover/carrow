#include "unix.h"
#include "addr.h"

#include <clog.h>

#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define UNIXBACKLOG 2


int
unix_listen(const char *sockfile) {
    int fd;
    int res;
    int option = 1;
    struct sockaddr_un addr;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, sockfile);
    
    /* Create socket */
    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);

    /* Allow reuse the address */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* Bind to unix port */
    res = bind(fd, &addr, sizeof(addr)); 
    if (res) {
        ERROR("Cannot bind on: %s", sockfile);
        return -1;
    }

    /* Listen */
    res = listen(fd, UNIXBACKLOG); 
    INFO("Listening on: %s", sockfile);
    if (res) {
        ERROR("Cannot listen on: %s", sockfile);
        return -1;
    }
    return fd;
}
