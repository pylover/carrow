#ifndef CARROW_H
#define CARROW_H


#include "core.h"
#include "ev.h"


/* Helper macros */
#define NEW_A(e) newA((failA)(e))
#define APPEND_A(c, state, priv) appendA(c, (taskA)(state), (void*)(priv))


#endif
