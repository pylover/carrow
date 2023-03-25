#include "carrow.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <err.h>


enum elementtypeA {
    MLETSIMPLE = 1,
    MLERFORK   = 2
};


struct circuitA {
    failA err;
    struct elementA *current;
    struct elementA *nets;
    void *ptr;
};



struct forkA {
    struct elementA *nextleft;
    struct elementA *nextright;
};


struct simpleA {
    struct elementA *next;
};


struct elementA {
    enum elementtypeA type;
    taskA run;
    void *priv;
    bool last;
    union {
        struct simpleA simple;
        struct forkA fork;
    };
};


struct circuitA * 
newA(failA error) {
    struct circuitA *c = malloc(sizeof(struct circuitA));
    if (c == NULL) {
        err(EXIT_FAILURE, "Out of memory");
    }
    
    c->err = error;
    c->current = NULL;
    c->nets = NULL;
    return c;
}


/** 
  Make elementA e2 from conputation and bind it to c. returns e2.

  c     e2   result

  o-o   O       o-o-O 

  o-o   O       o-o-O
  |_|           |___|

*/
struct elementA * 
appendA(struct circuitA *c, taskA f, void *priv) {
    struct elementA *e2 = malloc(sizeof(struct elementA));
    if (e2 == NULL) {
        err(EXIT_FAILURE, "Out of memory");
    }
    
    /* Initialize new elementA */
    e2->type = MLETSIMPLE;
    e2->run = f;
    e2->priv = priv;
    e2->last = true;
    e2->simple.next = NULL;
    
    if (c->nets == NULL) {
        c->nets = e2;
    }
    else {
        /* Bind them */
        bindA(c->nets, e2);
    }
    return e2;
}


static void 
freeE(struct elementA *e) {
    if (e == NULL) {
        return;
    }
    
    bool last = e->last;
    struct elementA *next = e->simple.next;
    free(e);

    if (last) {
        /* Disposition comppleted. */
        return;
    }

    freeE(next);
}


void 
freeA(struct circuitA *c) {
    if (c == NULL) {
        return;
    }
    
    freeE(c->nets);
    free(c);
}

/** 
  Bind two circuitAs:

  e1    e2     result

  o-o   O-O    o-o-O-O 

  o-o   O-O    o-o-O-O 
        |_|        |_|

  o-o   O-O    o-o-O-O
  |_|          |_____|

  o-o   O-O    o-o-O-O
  |_|   |_|    |_____|

*/
void 
bindA(struct elementA *e1, struct elementA *e2) {
    struct elementA *e1last = e1;
    struct elementA *e2last = e2;

    while (true) {
        /* Open cicuit */
        if (e1last->simple.next == NULL) {
            /* e1 Last elementA */
            e1last->simple.next = e2;
            e1last->last = false;
            return;
        }

        /* Closed cicuit */
        if (e1last->simple.next == e1) {
            /* It's a closed loop, Inserting e2 before the first elementA. */
            e1last->simple.next = e2;
            e1last->last = false;
            while (true) {
                /* e2 Last elementA */
                if ((e2last->simple.next == NULL) || 
                        (e2last->simple.next == e2)) {
                    e2last->simple.next = e1;
                    return;
                }
            
                /* Navigate forward */
                e2last = e2last->simple.next;
            }
            return;
        }
        
        /* Try next circuitA */
        e1last = e1last->simple.next;
    }
}


/** 
  Close (Loop) the circuitA c.

  Syntactic sugar for bindA(e2, e1) if e1 is the first elementA in the 
  circuitA and e2 is the last one.
    
  If the c1 is already a closed circuitA, then 1 will be returned.
  Otherwise the returned value will be zero.

  c     result

  o-o   o-o
        |_|

  o-o   err 
  |_|   

*/
int 
loopA(struct elementA *e) {
    struct elementA *first = e;
    struct elementA *last = e;
    while (true) {
        if (last->simple.next == NULL) {
            /* Last elementA */
            last->simple.next = first;
            last->last = true;
            return OK;
        }

        if (last->simple.next == first) {
            /* It's already a closed loop, Do nothing. */
            return ERR;
        }

        last = last->simple.next;
    }
}


void 
returnA(struct circuitA *c, void *state) {
    struct elementA *curr = c->current;
    if (curr->simple.next == NULL) {
        c->current = NULL;
        return;
    }

    struct elementA *next = curr->simple.next;
    c->current = next;
    next->run(c, state, next->priv);
}


void 
errorA(struct circuitA *c, void *state, const char *format, ...) {
    char buff[CARROW_ERRORMSG_BUFFSIZE]; 
    char *msg;
    va_list args;

    /* var args */
    if (format != NULL) {
        va_start(args, format);
        vsnprintf(buff, CARROW_ERRORMSG_BUFFSIZE, format, args);
        va_end(args);
        msg = buff;
    }
    else {
        msg = NULL;
    }

    if (c->err != NULL) {
        c->err(c, state, msg);
    }
    c->current = NULL;
}


void
runA(struct circuitA *c, void *state) {
    if (c->current == NULL) {
        c->current = c->nets;
    }
    c->current->run(c, state, c->current->priv);
}


void *
privA(struct circuitA *c) {
    return c->current->priv;
}
