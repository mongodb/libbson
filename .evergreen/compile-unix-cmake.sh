#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

CMAKE=/opt/cmake/bin/cmake

if command -v gtar 2>/dev/null; then
   TAR=gtar
else
   TAR=tar
fi

SRCROOT=`pwd`

BUILD_DIR=$(pwd)/cbuild
rm -rf $BUILD_DIR
mkdir $BUILD_DIR

INSTALL_DIR=$(pwd)/install-dir
rm -rf $INSTALL_DIR
mkdir -p $INSTALL_DIR

cd $BUILD_DIR
$TAR xf ../../libbson.tar.gz -C . --strip-components=1
$CMAKE -Wno-dev -Werror -Wdeprecated --warn-uninitialized --warn-unused-vars -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR .
make
make install

# Test our CMake package config file
cd $SRCROOT/examples/cmake/find_package
$CMAKE -DCMAKE_PREFIX_PATH=$INSTALL_DIR/lib/cmake .
make
LD_LIBRARY_PATH=$INSTALL_DIR/lib ./hello_bson

cd $SRCROOT/examples/cmake/FindPkgConfig
PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig /opt/cmake/bin/cmake .
make
LD_LIBRARY_PATH=$INSTALL_DIR/lib ./hello_bson
