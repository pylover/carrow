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

##### carrow_init() => int
Initialize carrow and event loop.

##### carrow_deinit() => void
Deinitialize carrow and event loop.


##### carrow_evloop(status*) => int
Block and run event loop.


#### Generic Functions

`foo.h`
```C
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

##### foo_coro_new(resolver, rejector) => foo_coro
Creates new coroutine

##### foo_coro_new_run(resolver, rejector, foo*) => void
Syntactic sugar for run(new(...), state).

##### foo_coro_from(foo_coro*, resolver) => foo_coro
Creates new coroutine from another by preserving the rejector.

##### foo_coro_stop() => foo_coro
A special coroutine used to stop the arrow thread execution.

##### foo_coro_evloop_add(foo_coro*, foo*, event*, fd, op) => int
Register pair of file descriptor and coro for one or more IO events.

##### foo_coro_evloop_del(fd) => int
Unregister filedescriptor from event loop.

##### foo_coro_run(foo_coro*, foo*) => void
Executes a coroutine.

##### foo_coro_reject(foo_coro*, foo*, errno, DBG, format*, ...) => foo_coro
It shouled be called by user to raise exception.


