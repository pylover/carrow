# carrow

C non-blocking IO framework inspired by Haskell's arrow and monads.


*From haskell's [wiki](https://wiki.haskell.org/Arrow):*
> **Arrows, or Freyd-categories, are a generalization of Monads.**

> "They can do everything monads can do, and more. They are roughly 
> comparable to monads with a static component.


## Build

```bash
cd path/to/carrow
mkdir build
cd build
cmake ..
make clean all
```

### Running examples

#### Example Dependencies: 

- https://github.com/pylover/clog
- https://github.com/pylover/mrb


```bash
cd path/to/carrow/build
make run_tcpclient
make profile_tcpclient

make run_tcpserver
make profile_tcpserver

make run_timer
make profile_timer

make run_sleep
make profile_sleep
```

### Contirbution

```bash
pip install cpplint
make lint
```
