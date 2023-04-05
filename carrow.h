#ifndef CARROW_H
#define CARROW_H


#include <stdbool.h>


struct CCORO;
struct CSTATE;


typedef struct CCORO 
    (*CNAME(resolver)) (struct CCORO *self, struct CSTATE *state, int fd, 
            int op);


typedef struct CCORO 
    (*CNAME(rejector)) (struct CCORO *self, struct CSTATE *state, int fd, 
            int no);


#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


#define REJECT(c, s, fd, ...) \
    CNAME(reject) (c, s, fd, errno, __FILENAME__, __LINE__, __FUNCTION__, \
            __VA_ARGS__)


#endif
