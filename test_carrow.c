#include <cutest.h>


#undef CSTATE
#undef CCORO
#undef CNAME
#undef CARROW_H


struct foobar {
    int all;
    int foo;
    int bar;
};


#define CSTATE  foobar
#define CCORO   foobarcoro
#define CNAME(n) foobarcoro_ ## n
#include "carrow.c"


#define CHUNK_SIZE  256


struct foobarcoro
barA(struct foobarcoro *self, struct foobar *s);


struct foobarcoro
errorA(struct foobarcoro *self, struct foobar *s, int no) {
    return foobarcoro_stop();
}


struct foobarcoro
fooA(struct foobarcoro *self, struct foobar *s) {
    if (s->all >= 10) {
        return REJECT(self, s, "All done");
    }
    s->foo++;
    s->all++;
    return foobarcoro_from(self, barA);
}


struct foobarcoro
barA(struct foobarcoro *self, struct foobar *s) {
    s->bar++;
    s->all++;
    return foobarcoro_from(self, fooA);
}


void
test_foo_loop() {
    static char b[CHUNK_SIZE];
    struct foobar state = {0, 0, 0};
    
    foobarcoro_run(fooA, errorA, &state);
    eqint(10, state.all);
    eqint(5, state.foo);
    eqint(5, state.bar);
}


void main() {
    test_foo_loop();
}
