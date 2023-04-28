#ifndef CARROW_H
#define CARROW_H


#include <stdbool.h>


struct CCORO;


typedef struct CCORO 
    (*CNAME(resolver)) (struct CCORO *self, struct CSTATE *state);


typedef struct CCORO 
    (*CNAME(rejector)) (struct CCORO *self, struct CSTATE *state, int no);


void
CNAME(resolve) (struct CCORO *self, struct CSTATE *s);


#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


#define DBG errno, __FILENAME__, __LINE__, __FUNCTION__


#define REJECT(c, s, ...) CNAME(reject) (c, s, DBG, __VA_ARGS__)

#endif
