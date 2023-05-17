#include <stdbool.h>


#ifndef CARROW_H
#error "carrow.h must be imported before importing the carrow_generic.h"
#error "And also #undef and #define CARROW_ENTITY before importing the " \
    "carrow_generic.h"
#endif


typedef struct CARROW_NAME(coro) CARROW_NAME(coro);


typedef CARROW_NAME(coro) (*CARROW_NAME(coro_resolver)) 
    (CARROW_NAME(coro) *self, CARROW_ENTITY *state);


typedef CARROW_NAME(coro) (*CARROW_NAME(coro_rejector)) 
    (CARROW_NAME(coro) *self, CARROW_ENTITY *state, int no);


void
CARROW_NAME(resolve) (CARROW_NAME(coro) *self, CARROW_ENTITY *s);
