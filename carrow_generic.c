#include "carrow.h"

#include <clog.h>

#include <stdarg.h>
#include <errno.h>


/* Core */
struct CARROW_NAME(coro) {
    CARROW_NAME(resolver) resolve;
    CARROW_NAME(rejector) reject;
};


CARROW_NAME(coro)
CARROW_NAME(coro_create) (CARROW_NAME(resolver) f, CARROW_NAME(rejector) r) {
    return (CARROW_NAME(coro)){f, r};
}


CARROW_NAME(coro)
CARROW_NAME(coro_create_from) (CARROW_NAME(coro) *base, 
        CARROW_NAME(resolver) f) {
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
CARROW_NAME(coro_run) (CARROW_NAME(coro) *self, CARROW_ENTITY *s) {
    CARROW_NAME(coro) c = *self;
   
    while (c.resolve != NULL) {
        c = c.resolve(&c, s);
    }
}


void
CARROW_NAME(coro_create_and_run) (CARROW_NAME(resolver) f, CARROW_NAME(rejector) r, 
        CARROW_ENTITY *state) {
    CARROW_NAME(coro) c = CARROW_NAME(coro_create)(f, r);
    CARROW_NAME(coro_run)(&c, state);
}


static void
CARROW_NAME(event_handler) (CARROW_NAME(coro) *self, CARROW_ENTITY *s, 
        enum carrow_event_status status) {
    CARROW_NAME(coro) c = *self;
    int eno;
   
    if (status != CES_OK) {
        if (status == CES_ERR) {
            eno = errno;
        }
        else {
            eno = EINTR;
        }
        c.reject(self, s, eno);
        return;
    }
    
    CARROW_NAME(coro_run) (self, s);
}


int
CARROW_NAME(coro_evloop_register) (CARROW_NAME(coro) *c, CARROW_ENTITY *s, 
        struct carrow_event *e, int fd, int op) {
    e->fd = fd;
    e->op = op;
    return carrow_evloop_register(c, s, e, 
            (carrow_event_handler)CARROW_NAME(event_handler));
}


int
CARROW_NAME(coro_evloop_unregister) (int fd) {
    return carrow_evloop_unregister(fd);
}
