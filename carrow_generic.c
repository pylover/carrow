#include "carrow.h"

#include <clog.h>

#include <stdarg.h>
#include <errno.h>


CARROW_NAME(coro)
CARROW_NAME(coro_create) (CARROW_NAME(coro_resolver) f, 
        CARROW_NAME(coro_rejector) r) {
    return (CARROW_NAME(coro)){f, r};
}


CARROW_NAME(coro)
CARROW_NAME(coro_create_from) (CARROW_NAME(coro) *base, 
        CARROW_NAME(coro_resolver) f) {
    return (CARROW_NAME(coro)){f, base->reject};
}


CARROW_NAME(coro)
CARROW_NAME(coro_stop) () {
    return (CARROW_NAME(coro)){NULL, NULL};
}


CARROW_NAME(coro)
CARROW_NAME(coro_reject) (CARROW_NAME(coro) *self, CARROW_ENTITY *s, int no, 
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
    return CARROW_NAME(coro_stop)();
}


void
CARROW_NAME(coro_run) (CARROW_NAME(coro) *self, CARROW_ENTITY *s, int fd, 
        int events) {
    CARROW_NAME(coro) c = *self;
   
    while (c.resolve != NULL) {
        // TODO: any 
        c = c.resolve(&c, s, fd, events);
        // TODO: any 
    }
}


void
CARROW_NAME(coro_create_and_run) (CARROW_NAME(coro_resolver) f, 
        CARROW_NAME(coro_rejector) r, CARROW_ENTITY *state, int fd, 
        int events) {
    CARROW_NAME(coro) c = CARROW_NAME(coro_create)(f, r);
    CARROW_NAME(coro_run)(&c, state, fd, events);
}


int
CARROW_NAME(evloop_register) (CARROW_NAME(coro) *c, CARROW_ENTITY *s, int fd, 
        int events) {
    return carrow_evloop_register(c, s, fd, events,
            (carrow_generic_coro_resolver)CARROW_NAME(coro_run));
}


int
CARROW_NAME(evloop_modify) (CARROW_NAME(coro) *c, CARROW_ENTITY *s, int fd, 
        int events) {
    return carrow_evloop_modify(c, s, fd, events,
            (carrow_generic_coro_resolver)CARROW_NAME(coro_run));
}


int
CARROW_NAME(evloop_modify_or_register) (CARROW_NAME(coro) *c, 
        CARROW_ENTITY *s, int fd, int events) {
    return carrow_evloop_modify_or_register(c, s, fd, events,
            (carrow_generic_coro_resolver)CARROW_NAME(coro_run));
}


int
CARROW_NAME(evloop_unregister) (int fd) {
    return carrow_evloop_unregister(fd);
}
