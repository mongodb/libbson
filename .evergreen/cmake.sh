#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail


if command -v gtar 2>/dev/null; then
   TAR=gtar
else
   TAR=tar
fi
$TAR xf ../libbson.tar.gz -C . --strip-components=1

SRCROOT=`pwd`
mkdir cmake
cd cmake
/opt/cmake/bin/cmake -Wno-dev -Werror -Wdeprecated --warn-uninitialized --warn-unused-vars -DCMAKE_INSTALL_PREFIX=`pwd` ..
make
make test
make install

# Test our CMake package config file
cd $SRCROOT/examples/cmake/find_package
/opt/cmake/bin/cmake -DCMAKE_PREFIX_PATH=$SRCROOT/cmake/lib/cmake .
make
LD_LIBRARY_PATH=$SRCROOT/cmake ./hello_bson

cd $SRCROOT/examples/cmake/FindPkgConfig
PKG_CONFIG_PATH=$SRCROOT/cmake /opt/cmake/bin/cmake .
make
LD_LIBRARY_PATH=$SRCROOT/cmake ./hello_bson
