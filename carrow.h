#ifndef CARROW_H
#define CARROW_H


#include "core.h"
#include "ev.h"


/* Helper macros */
#define NEW_A(e) newA((failA)(e))
#define APPEND_A(c, state, priv) appendA(c, (taskA)(state), (void*)(priv))
#define RETURN_A(c, state) returnA(c, state)
#define ERROR_A(c, state, ...) errorA(c, state, __VA_ARGS__)
#define RUN_A(c, state) runA(c, state)


#endif
