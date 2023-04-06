#include "carrow.h"

#include <clog.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/epoll.h>


/* Core */
struct CCORO {
    CNAME(resolver) resolve;
    CNAME(rejector) reject;
};


struct CCORO
CNAME(new) (CNAME(resolver) f, CNAME(rejector) r) {
    return (struct CCORO){f, r};
}


struct CCORO
CNAME(from) (struct CCORO *base, CNAME(resolver) f) {
    return (struct CCORO){f, base->reject};
}


struct CCORO
CNAME(ok) () {
    return (struct CCORO){NULL, NULL};
}


struct CCORO
CNAME(failed) () {
    return (struct CCORO){NULL, NULL};
}


struct CCORO
CNAME(reject) (struct CCORO *self, struct CSTATE *s, int fd, int no,
        const char *filename, int lineno, const char *function,
        const char *format, ... ) {
    va_list args;

    if (format) { 
        va_start(args, format);
    }

    clog_vlog(
            CLOG_ERROR,
            filename, 
            lineno, 
            function, 
            true, 
            format, 
            args
        );

    if (format) { 
        va_end(args);
    }
   
    if (self->reject != NULL) {
        return self->reject(self, s, fd, no);
    }

    errno = 0;
    return CNAME(failed)();
}


/* event loop bags */
struct CNAME(evbag) {
    struct CCORO coro;
    struct CSTATE *state;
    int fd;
};


static struct CNAME(evbag) *
CNAME(evbag_new) (struct CCORO *self, struct CSTATE *state, int fd) {
    struct CNAME(evbag) *bag = malloc(sizeof(struct CNAME(evbag)));
    if (bag == NULL) {
        FATAL("Out of memory");
    }

    bag->coro = *self;
    bag->state = state;
    bag->fd = fd;
    return bag;
}


/* Event loop */
static int epollfd;


int
CNAME(arm) (struct CCORO *c, struct CSTATE *s, int fd, int op) {
    struct epoll_event ev;
    struct CNAME(evbag) *bag = CNAME(evbag_new)(c, s, fd);
    
    ev.events = op;
    ev.data.ptr = bag;
    
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev)) {
        if (errno == ENOENT) {
            errno = 0;
            return epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
        }

        return -1;
    }

    return 0;
}


void
CNAME(run) (CNAME(resolver) f, CNAME(rejector) r, struct CSTATE *state) {
    struct CCORO c = {f, r};

    while (c.resolve != NULL) {
        c = c.resolve(&c, state, -1, 0);
    }
}


int
CNAME(loop) (volatile int *status, struct CSTATE *state) {
    int i;
    int nfds;
    int ret = 0;
    // struct epoll_event ev;
    // struct epoll_event events[EVMAX];

    // while ((status == NULL) || (*status > EXIT_FAILURE)) {
    //     nfds = epoll_wait(epollfd, events, evmax, -1);
    //     if (nfds < 0) {
    //         ret = -1;
    //         break;
    //     }

    //     if (nfds == 0) {
    //         ret = 0;
    //         break;
    //     }

    //     for (i = 0; i < nfds; i++) {
    // //         ev = events[i];
    // //         bag = (struct evbag *) ev.data.ptr;
    // //         c = bag->circuit;
    // //         s = bag->state;
    // //         fd = s->fd;

    // //         s->events = ev.events;
    // //         DEBUG("Continue");
    // //         enum circuitstatus st = continueA(c, bag->current, s);
    // //         switch (st) {
    // //             case CSDISPOSE:
    // //                 DEBUG("DISPOSE");
    // //                 evdearm(fd);
    // //                 break;
    // //             case CSSTOP:
    // //                 DEBUG("STOP");
    // //                 evdearm(fd);
    // //                 break;
    // //             case CSOK:
    // //                 DEBUG("OK");
    // //                 break;
    // //         }
    //     }
    // }

    return ret;
}
