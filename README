# libbson

libbson is a library providing useful routines related to building, parsing,
and iterating BSON documents.  It is a useful base for those wanting to write
high-performance C extensions to higher level languages such as python, ruby,
or perl.


# Building

## Dependencies

### Fedora / RedHat Enterprise / CentOS

```sh
yum install git automake autoconf libtool gcc
```

### Debian / Ubuntu

```sh
apt-get install git-core automake autoconf libtool gcc
```

### FreeBSD

```sh
pkg install git automake autoconf libtool gcc pkgconf
```

### SmartOS

```sh
pkgin install git automake autoconf libtool gcc47 gmake pkg-config
export PATH=/opt/local/gcc47/bin:$PATH
```

### Windows Vista and Higher

Builds on Windows Vista and Higher require cmake to build Visual Studio project files.
Alternatively, you can use cygwin or mingw with the automake based build.

```sh
git clone git://github.com/mongodb/libbson.git
cd libbson
cmake.exe build\win32
msbuild.exe ALL_BUILD.vcxproj
```

For the adventurous, you can cross-compile for Windows from Fedora easily using mingw.

```sh
./configure --host=x86_64-w64-mingw32
```

## From Git

```sh
$ git clone git://github.com/mongodb/libbson.git
$ cd libbson/
$ ./autogen.sh --enable-silent-rules
$ make
$ sudo make install
```

You can run the unit tests with

```sh
make test
```

## From Tarball

```sh
tar xzf libbson-$ver.tar.gz
./configure --enable-silent-rules
make
sudo make install
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

# Examples

See the `examples/` directory for how to use the libbson library in your
application.

# Documentation

See the `doc/` directory for documentation on individual types.

