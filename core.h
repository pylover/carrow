#ifndef CARROW_CORE_H
#define CARROW_CORE_H


#define OK  0
#define ERR 1
#define CARROW_ERRORMSG_BUFFSIZE    1024


struct elementA;
struct circuitA;


typedef struct elementA * (*taskA) (struct circuitA *a, void *state, 
        void *priv);
typedef void (*okA) (struct circuitA *a, void* state);
typedef void (*failA) (struct circuitA *a, void* state, const char *msg);


struct circuitA *
newA(failA errcb);


void
freeA(struct circuitA *a);


struct elementA *
currentA(struct circuitA *c);


void
bindA(struct elementA *e1, struct elementA *e2);


struct elementA *
appendA(struct circuitA *c, taskA f, void *priv);


int
loopA(struct elementA *e);


struct elementA*
nextA(struct circuitA *c, void *state);


struct elementA*
errorA(struct circuitA *c, void *state, const char *format, ...);


int
runA(struct circuitA *c, void *state);


void *
privA(struct circuitA *c);


int
continueA(struct circuitA *c, struct elementA *el, void *state);


#endif
