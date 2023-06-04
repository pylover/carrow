#include "carrow.h"

#include <clog.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/timerfd.h>


typedef struct foo {
    unsigned int counter;
} foo;


#undef CARROW_ENTITY
#define CARROW_ENTITY foo
#include "carrow_generic.h"
#include "carrow_generic.c"


static void
fooA(struct foo_coro *self, struct foo *state) {
    CORO_START;
    
    while (true) {
        INFO("Flip: %u", state->counter++);
        CORO_SLEEP(1); // One second

        INFO("Flop: %u", state->counter++);
        CORO_SLEEP(1); // One second
    }

    CORO_FINALLY;
    CORO_CLEANUP;
}


int
main() {
    clog_verbosity = CLOG_DEBUG;

    if (carrow_handleinterrupts()) {
        return EXIT_FAILURE;
    }
    
    struct foo state = {
        .counter = 0
    };

    return foo_forever(fooA, &state, NULL);
}
