#include "cutest.h"
#include "carrow.h"


#define CHUNK_SIZE  256


struct state {
    int all;
    int foo;
    int bar;
    int baz;
    int qux;
    char error[CHUNK_SIZE];
};


void
fooA(struct circuitA *c, struct state *s) {
    s->foo++;
    s->all++;
    RETURN_A(c, s);
}


void
barA(struct circuitA *c, struct state *s) {
    s->bar++;
    s->all++;
    RETURN_A(c, s);
}


void
bazA(struct circuitA *c, struct state *s) {
    s->baz++;
    s->all++;
    RETURN_A(c, s);
}


void
quxA(struct circuitA *c, struct state *s) {
    s->qux++;
    s->all++;
    RETURN_A(c, s);
}


void
test_fork() {
    static char b[CHUNK_SIZE];
    struct state state = {0, 0, 0, false, '\0'};
    struct circuitA *c = NEW_A(NULL);
    struct elementA *e = APPEND_A(c, fooA, NULL);
    struct elementA *f =   FORK_A(c, barA, NULL, bazA, NULL);
    struct elementA *j =   JOIN_A(c, forkA);
    struct elementA *e = APPEND_A(c, quxA, NULL);
    
    RUN_A(c, &state);
    eqint(10, state.all);
    eqint(10, state.foo);
    eqstr("All done", state.error);
    freeA(c);
}


void 
main() {
    test_fork();
}
