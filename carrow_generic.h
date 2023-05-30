#ifndef CARROW_H
#error "carrow.h and clog.h must be imported before importing the" \
    "carrow_generic.h"
#error "And also #undef and #define CARROW_ENTITY before importing the " \
    "carrow_generic.h"
#else


#include <stdbool.h>


typedef struct CARROW_NAME(coro) CARROW_NAME(coro);


typedef CARROW_NAME(coro) (*CARROW_NAME(coro_resolver)) 
    (CARROW_NAME(coro) *self, CARROW_ENTITY *state, int fd, int events);


typedef CARROW_NAME(coro) (*CARROW_NAME(coro_rejector)) 
    (CARROW_NAME(coro) *self, CARROW_ENTITY *state, int errorno);


struct CARROW_NAME(coro) {
    CARROW_NAME(coro_resolver) resolve;
    CARROW_NAME(coro_rejector) reject;
};


void
CARROW_NAME(resolve) (CARROW_NAME(coro) *self, CARROW_ENTITY *s);


CARROW_NAME(coro)
CARROW_NAME(coro_create) (CARROW_NAME(coro_resolver) f, 
        CARROW_NAME(coro_rejector) r);


CARROW_NAME(coro)
CARROW_NAME(coro_create_from) (CARROW_NAME(coro) *base, 
        CARROW_NAME(coro_resolver) f);


CARROW_NAME(coro)
CARROW_NAME(coro_stop) ();


CARROW_NAME(coro)
CARROW_NAME(coro_reject) (CARROW_NAME(coro) *self, CARROW_ENTITY *s, 
        int errorno, const char *filename, int lineno, const char *function, 
        const char *format, ... );


void
CARROW_NAME(coro_run) (CARROW_NAME(coro) *self, CARROW_ENTITY *s, int fd, 
        int events);


void
CARROW_NAME(coro_create_and_run) (CARROW_NAME(coro_resolver) f, 
        CARROW_NAME(coro_rejector) r, CARROW_ENTITY *state, int fd, 
        int events);


int
CARROW_NAME(evloop_register) (CARROW_NAME(coro) *c, CARROW_ENTITY *s, 
        int fd, int events);


int
CARROW_NAME(evloop_modify) (CARROW_NAME(coro) *c, CARROW_ENTITY *s, int fd, 
        int events);


int
CARROW_NAME(evloop_modify_or_register) (CARROW_NAME(coro) *c, 
        CARROW_ENTITY *s, int fd, int events);


int
CARROW_NAME(evloop_unregister) (int fd);


int
CARROW_NAME(forever) (CARROW_NAME(coro_resolver) resolve, 
        CARROW_NAME(coro_rejector) reject, CARROW_ENTITY *state, 
        volatile int *status);


#endif
