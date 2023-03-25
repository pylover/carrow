# carrow
C arrow library


## Usage

Link the following example with `-lcarrow`.

```C
#include <carrow.h>
```

## Build & Install

```bash
mkdir build
cd build
cmake ..
make
```

### Profile


```
sudo apt install valgrind
make all profile
```

### Install with makefile

```bash
cd build
make install
```


### Create debian package

```bash
cd build
cpack
```

After that, `libcarrow-*.deb` will be generated insode the `build` directory.

#### Install using debian package

```bash
cd build
sudo dpkg -i libcarrow-*.deb
```

##### Uninstall

```bash
sudo dpkg -P libcarrow
```

Or

```bash
sudo apt remove libcarrow
```
