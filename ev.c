#include "ev.h"


struct evbag {
    int fd;
    struct circuitA *circuit;
    void *state;
};

