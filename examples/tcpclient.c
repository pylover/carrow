
#undef CSTYPE
#undef CNAME
#undef CARROW_CORE_H


struct foobar {
    int all;
    int foo;
    int bar;
};


#define CSTYPE  struct foobar *
#define CSNAME  foobarcoro
#define CNAME(n) foobarcoro_ ## n

#include "carrow.c"


int
main() {
    return 0;
}
