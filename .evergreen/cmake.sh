#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail


if command -v gtar 2>/dev/null; then
   TAR=gtar
else
   TAR=tar
fi
$TAR xf ../libbson.tar.gz -C . --strip-components=1
mkdir cmake
cd cmake
/opt/cmake/bin/cmake -Wno-dev -Werror -Wdeprecated --warn-uninitialized --warn-unused-vars  ../.
make
make test
