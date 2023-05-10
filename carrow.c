#include "carrow.h"
#include "evloop.h"

#include <clog.h>

#include <stdarg.h>
#include <errno.h>


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
CNAME(stop) () {
    return (struct CCORO){NULL, NULL};
}


struct CCORO
CNAME(reject) (struct CCORO *self, CSTATE *s, int no,
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
        return self->reject(self, s, no);
    }

    errno = 0;
    return CNAME(stop)();
}


static void
CNAME(evhandler) (struct CCORO *self, CSTATE *s, 
        enum ev_status status) {
    struct CCORO c = *self;
    int eno;
   
    if (status != EE_OK) {
        if (status == EE_ERR) {
            eno = errno;
        }
        else {
            eno = EINTR;
        }
        c.reject(self, s, eno);
        return;
    }

    while (c.resolve != NULL) {
        c = c.resolve(&c, s);
    }
}


int
CNAME(wait) (struct CCORO *c, CSTATE *s, struct event *e, int fd, int op) {
    e->fd = fd;
    e->op = op;
    return ev_wait(c, s, e, (ev_handler)CNAME(evhandler));
}


int
CNAME(nowait) (int fd) {
    return ev_nowait(fd);
}


void
CNAME(resolve) (struct CCORO *self, CSTATE *s) {
    struct CCORO c = *self;
   
    while (c.resolve != NULL) {
        c = c.resolve(&c, s);
    }
}


void
CNAME(run) (CNAME(resolver) f, CNAME(rejector) r, CSTATE *state) {
    struct CCORO c = {f, r};

    CNAME(resolve)(&c, state);
}


int
CNAME(runloop) (CNAME(resolver) f, CNAME(rejector) r, CSTATE *state,
        volatile int *status) {
    CNAME(run)(f, r, state);
    return ev_loop(status);
}
