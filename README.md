# Libbson

libbson is a library providing useful routines related to building, parsing,
and iterating BSON documents.


## Building

Detailed installation instructions are in the manual:
http://api.mongodb.org/libbson/current/installing.html

### From Tarball

```sh
tar xzf libbson-$ver.tar.gz
./configure
make
sudo make install
```

### From Git

```sh
git clone git://github.com/mongodb/libbson.git
cd libbson/
./autogen.sh
make
sudo make install
```

## Configuration Options

You may be interested in the following options for `./configure`.
These are not availble when using the alternate CMake build system.

```
--help                    Show all possible help options.
                          There are many more than those displayed here.

--enable-optimizations    Enable various compile and link optimizations.
--enable-debug            Enable debugging features.
--enable-debug-symbols    Link with debug symbols in tact.
--enable-hardening        Enable stack protector and other hardening features.
--enable-silent-rules     Force silent rules like the Linux kernel.
--enable-coverage         Compile with gcov support.
--enable-static           Build static archives (.a).
```

# Developing using libbson

In your source code:

```c
#include <bson.h>
```

To get the include path and libraries appropriate for your system.

```sh
gcc my_program.c $(pkg-config --cflags --libs libbson-1.0)
```

## Examples

See the `examples/` directory for how to use the libbson library in your
application.

## Documentation

See the `doc/` directory for documentation on individual types.

## Bug reports and Feature requests

Please use [the MongoDB C Driver JIRA project](https://jira.mongodb.org/browse/CDRIVER) to report bugs or request features.
