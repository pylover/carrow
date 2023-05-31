#include "carrow.h"
#include <cutest.h>


typedef struct foo {
    int all;
    int foo;
    int bar;
} foo;


#undef CARROW_ENTITY
#define CARROW_ENTITY foo
#include "carrow_generic.h"
#include "carrow_generic.c"


void
barA(struct foo_coro *self, struct foo *s);


void
fooA(struct foo_coro *self, struct foo *s) {
    if (s->all >= 10) {
        self->run = NULL;
        return;
    }
    s->foo++;
    s->all++;
    self->run = barA;
}


void
barA(struct foo_coro *self, struct foo *s) {
    s->bar++;
    s->all++;
    self->run = fooA;
}


void
test_foo_loop() {
    static char b[256];
    struct foo state = {0, 0, 0};
    
    foo_coro_create_and_run(fooA, &state);
    eqint(10, state.all);
    eqint(5, state.foo);
    eqint(5, state.bar);
}


void main() {
    test_foo_loop();
}
