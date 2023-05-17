# carrow

C non-blocking IO framework inspired by Haskell's arrow and monads.

*From haskell's [wiki](https://wiki.haskell.org/Arrow):*
> **Arrows, or Freyd-categories, are a generalization of Monads.**

> "They can do everything monads can do, and more. They are roughly 
> comparable to monads with a static component.


#### Types

```C
struct carrow_event {
    int fd;
    int op;
};
```


#### Functions

```C
#include <carrow.h>
```

##### int carrow_init()
Initialize carrow and event loop.

##### void carrow_deinit()
Deinitialize carrow and event loop.


##### int carrow_evloop(status*)
Block and run event loop.


#### Generic Functions

`foo.h`
_from```C
#ifndef FOO_H
#define FOO_H


typedef struct foo {
    int all;
    int foo;
    int bar;
} foo;


#undef CARROW_ENTITY
#define CARROW_ENTITY foo
#include "carrow_generic.h"


#endif
```

`foo.c`
```C
#undef CARROW_ENTITY
#define CARROW_ENTITY foo
#include "foo.h"
#include "carrow_generic.c"
```

##### foo_coro foo_coro_create(resolver, rejector)
Creates new coroutine

##### void foo_coro_create_and_run(resolver, rejector, foo*)
Syntactic sugar for foo_coro_run(foo_coro_create(...), state).

##### foor_coro foo_coro_from(foo_coro*, resolver)
Creates new coroutine from another by preserving the rejector.

##### foo_coro foo_coro_stop()
A special coroutine used to stop the arrow thread execution.

##### int foo_evloop_register(foo_coro*, foo*, event*, fd, op)
Register pair of file descriptor and coro for one or more IO events.

##### int foo_evloop_unregister(fd)
Unregister filedescriptor from event loop.

##### void foo_coro_run(foo_coro*, foo*)
Executes a coroutine.

##### foo_coro foo_coro_reject(foo_coro*, foo*, errno, DBG, format*, ...)
It shouled be called by user to raise exception.
