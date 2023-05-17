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


// Initialize carrow and event loop.
int carrow_init()

// Deinitialize carrow and event loop.
void carrow_deinit()

// Block and run event loop.
int carrow_evloop(volatile int *status)
```


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


// Creates new coroutine
foo_coro 
foo_coro_create(foo_coro_resolver, foo_coro_rejector); 


// Syntactic sugar for foo_coro_run(foo_coro_create(...), state)
void 
foo_coro_create_and_run(foo_coro_resolver, foo_coro_rejector, foo* foo);


// Creates new coroutine from another by preserving the rejector.
foor_coro 
foo_coro_from(foo_coro *self, foo_coro_resolver);


// A special coroutine used to stop the arrow thread execution.
foo_coro 
foo_coro_stop();


// Register/Modify pair of file descriptor and coro for one or more IO events.
int 
foo_evloop_register(foo_coro*, foo*, carrow_event*, int fd, int op);

int 
foo_evloop_modify(foo_coro*, foo*, carrow_event*, int fd, int op);

int 
foo_evloop_modify_or_register(foo_coro*, foo*, carrow_event*, int fd, int op);


// Unregister filedescriptor from event loop.
int 
foo_evloop_unregister(int fd);


// Execute a coroutine.
void 
foo_coro_run(foo_coro*, foo*);


// It shouled be called by user to raise exception.
foo_coro 
foo_coro_reject(foo_coro*, foo*, int errno, DBG, const char *format, ...);
```
