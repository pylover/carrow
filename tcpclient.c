#include "carrow.h"
#include "tcp.h"
#include "addr.h"

#include <clog.h>
#include <mrb.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <termios.h>


#define WORKING 9999

#define PAGESIZE 4096
#define BUFFSIZE (PAGESIZE * 32768)


static volatile int status = WORKING;
static struct sigaction old_action;


struct connstate {
    struct tcpclientstate;

    struct mrb inbuff;
    struct mrb outbuff;

    size_t bytesin;
    size_t bytesout;
};



static struct termios ttysave;


void
stdcannonical() {
    tcsetattr(STDIN_FILENO, TCSANOW, &ttysave);
}


void 
stdnocannonical() {
    struct termios ttystate, ttysave;

    // get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);
    ttysave = ttystate;
    //turn off canonical mode and echo
    ttystate.c_lflag &= ~(ICANON | ECHO);
    // ttystate.c_lflag |= (ICANON | ECHO | ECHOE);
    // ttystate.c_iflag |= IGNCR;
    // ttystate.c_lflag &= ~ICANON;
    //minimum of number input read.
    ttystate.c_cc[VMIN] = 0;
    ttystate.c_cc[VTIME] = 0;

    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}


struct elementA *
_connerror(struct circuitA *c, struct connstate *s, const char *msg) {
    DEBUG("Closed: %d in: %lu, out: %lu: %s", s->fd, s->bytesin, s->bytesout,
            msg);
    close(s->fd);
    evdearm(s->fd);
    mrb_deinit(&(s->inbuff));
    mrb_deinit(&(s->outbuff));
    return stopA(c);
}


struct elementA *
_pipeA(struct circuitA *c, struct connstate *s) {
    int fd = s->fd;
    int events = s->events;
    struct mrb *outbuff = &(s->outbuff);
    struct mrb *inbuff = &(s->inbuff);
    size_t outbuffavail = mrb_space_available(outbuff);
    ssize_t bytes;
    
    DEBUG("pipe: %d", s->fd);
    if ((fd == STDIN_FILENO) && (events & EVIN)) {
        while (outbuffavail) {
            DEBUG("Reading");
            bytes = mrb_readin(outbuff, STDIN_FILENO, outbuffavail);
            DEBUG("IN: %lu", bytes);
            if (bytes < 0) {
                if (EVMUSTWAIT()) {
                    errno = 0;
                    break;
                }
                else {
                    return errorA(c, s, "stdin read error");
                }
            }
            else if (bytes == 0) {
                return errorA(c, s, "stdin read EOF");
            }
            else {
                outbuffavail -= bytes;
            }
        }
    }
    
    return evwaitA(c, (struct evstate*)s, STDIN_FILENO, EVIN | EVONESHOT);
}


void sighandler(int s) {
    PRINTE(CR);
    status = EXIT_SUCCESS;
}


void catch_signal() {
    struct sigaction new_action = {sighandler, 0, 0, 0, 0};
    if (sigaction(SIGINT, &new_action, &old_action) != 0) {
        FATAL("sigaction");
    }
}


int
main() {
    int ret = EXIT_SUCCESS;
    clog_verbosity = CLOG_DEBUG;
    // stdnocannonical();

    /* Signal */
    catch_signal();

    struct connstate state = {
        .host = "0.0.0.0",
        .port = "3030",

        .fd = STDIN_FILENO,
        .bytesin = 0,
        .bytesout = 0,
    };
    mrb_init(&(state.inbuff), BUFFSIZE);
    mrb_init(&(state.outbuff), BUFFSIZE);

    struct circuitA *c = NEW_A(_connerror);
                         APPEND_A(c, evinitA, NULL);
                         APPEND_A(c, connectA, NULL);
    struct elementA *e = APPEND_A(c, _pipeA, NULL);
               loopA(e); 

    runA(c, &state);
    if (evloop(&status)) {
        ret = EXIT_FAILURE;
    }
    evdeinitA();
    freeA(c);
    mrb_deinit(&(state.inbuff));
    mrb_deinit(&(state.outbuff));
    // stdcannonical();
    return ret;
}
