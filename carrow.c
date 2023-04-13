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
CNAME(reject) (struct CCORO *self, struct CSTATE *s, int no,
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


int
CNAME(arm) (struct CCORO *c, struct CSTATE *s, struct event *e) {
    return carrow_arm(c, s, e, (carrow_evhandler)CNAME(resolve));
}


int
CNAME(dearm) (int fd) {
    return carrow_dearm(fd);
}


void
CNAME(resolve) (struct CCORO *self, struct CSTATE *s) {
    struct CCORO c = *self;
   
    while (c.resolve != NULL) {
        c = c.resolve(&c, s);
    }
}


void
CNAME(run) (CNAME(resolver) f, CNAME(rejector) r, struct CSTATE *state) {
    struct CCORO c = {f, r};

    CNAME(resolve)(&c, state);
}


int
CNAME(loop) (volatile int *status) {
    return carrow_evloop(status);
}


int
CNAME(runloop) (CNAME(resolver) f, CNAME(rejector) r, struct CSTATE *state,
        volatile int *status) {
    CNAME(run)(f, r, state);
    return CNAME(loop)(status);
}
