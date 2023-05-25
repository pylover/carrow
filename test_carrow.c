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


struct foo_coro
barA(struct foo_coro *self, struct foo *s, int efd, int events);


struct foo_coro
errorA(struct foo_coro *self, struct foo *s, int no, int efd, int events) {
    return foo_coro_stop();
}


struct foo_coro
fooA(struct foo_coro *self, struct foo *s, int efd, int events) {
    if (s->all >= 10) {
        return foo_coro_reject(self, s, -1, -1, __DBG__, "All done");
    }
    s->foo++;
    s->all++;
    return foo_coro_create_from(self, barA);
}


struct foo_coro
barA(struct foo_coro *self, struct foo *s, int efd, int events) {
    s->bar++;
    s->all++;
    return foo_coro_create_from(self, fooA);
}


void
test_foo_loop() {
    static char b[256];
    struct foo state = {0, 0, 0};
    
    foo_coro_create_and_run(fooA, errorA, &state, -1, -1);
    eqint(10, state.all);
    eqint(5, state.foo);
    eqint(5, state.bar);
}


void main() {
    test_foo_loop();
}
