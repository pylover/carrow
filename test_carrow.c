#include "carrow.h"

#include <cutest.h>


#define CHUNK_SIZE  256


struct state {
    int all;
    int foo;
    int bar;
    char error[CHUNK_SIZE];
};


struct barstate {
    int counter;
};


void
fooA(struct circuitA *c, struct state *s) {
    s->foo++;
    s->all++;
    if (s->all >= 10) {
        ERROR_A(c, s, "All done");
        return;
    }
    RETURN_A(c, s);
}


void
barA(struct circuitA *c, struct state *s, struct barstate *bar) {
    s->bar++;
    s->all++;
    bar->counter++;
    RETURN_A(c, s);
}


void
oops(struct circuitA *c, struct state *s, const char *error) {
    strcpy(s->error, error);
}


void
test_foo_loop() {
    static char b[CHUNK_SIZE];
    struct state state = {0, 0, 0, false, '\0'};
    struct circuitA *c = NEW_A(oops);
    struct elementA *e = APPEND_A(c, fooA, NULL);
               loopA(e); 
    
    RUN_A(c, &state);
    eqint(10, state.all);
    eqint(10, state.foo);
    eqstr("All done", state.error);
    freeA(c);
}


void
test_foobar_loop() {
    static char b[CHUNK_SIZE];
    struct state state = {0, 0, 0, false, '\0'};
    struct barstate bar = {0};
    struct circuitA *c = NEW_A(oops);
    struct elementA *e = APPEND_A(c, fooA, NULL);
                         APPEND_A(c, barA, &bar);
               loopA(e); 
    
    RUN_A(c, &state);
    eqint(11, state.all);
    eqint(6, state.foo);
    eqint(5, state.bar);
    eqint(5, bar.counter);
    eqstr(state.error, "All done");
    freeA(c);
}


void main() {
    test_foo_loop();
    test_foobar_loop();
}
