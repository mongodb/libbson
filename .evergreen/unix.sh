#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

# Supported/used environment variables:
#       CFLAGS   Additional compiler flags
#       MARCH    Machine Architecture. Defaults to lowercase uname -m
#       PATH     Where runtime executable are located
#       RELEASE  Use the fully qualified release archive
#       DEBUG    Use debug configure flags
#       VALGRIND Run the test suite through valgrind
#       CC       Which compiler to use




# Get the kernel name, lowercased
OS=$(uname -s | tr '[:upper:]' '[:lower:]')

# Automatically retrieve the machine architecture, lowercase, unless provided
# as an environment variable (e.g. to force 32bit)
[ -z "$MARCH" ] && MARCH=$(uname -m | tr '[:upper:]' '[:lower:]')

# Default configure flags for debug builds and release builds
DEBUG_FLAGS="--enable-optimizations=no --enable-man-pages=no --enable-html-docs=no --enable-extra-align=no --enable-hardening=no --enable-maintainer-flags --enable-debug"
RELEASE_FLAGS="--enable-optimizations --enable-man-pages=no --enable-html-docs=no --enable-extra-align=no --enable-hardening"

# By default we build from git clone, which requires autotools
# This gets overwritten if we detect we should use the release archive
CONFIGURE_SCRIPT="./autogen.sh"


# --strip-components is an GNU tar extension. Check if the platform
# (e.g. Solaris) has GNU tar installed as `gtar`, otherwise we assume to be on
# platform that supports it
# command -v returns success error code if found and prints the path to it
if command -v gtar 2>/dev/null; then
   TAR=gtar
else
   TAR=tar
fi


case "$MARCH" in
   i386)
      CFLAGS="$CFLAGS -m32 -march=i386"
   ;;
   x86_64)
      CFLAGS="$CFLAGS -m64 -march=x86-64"
   ;;
   ppc64le)
      CFLAGS="$CFLAGS -mcpu=power8 -mtune=power8 -mcmodel=medium"
   ;;
esac


case "$OS" in
   darwin)
      CFLAGS="$CFLAGS -Wno-unknown-pragmas"
   ;;

   linux)
      # Make linux builds a tad faster by parallelise the build
      cpus=$(grep -c '^processor' /proc/cpuinfo)
      MAKEFLAGS="-j{$cpus}"
   ;;

   sunos)
      PATH="/opt/mongodbtoolchain/bin:$PATH"
   ;;
esac

case "$CC" in
   clang)
   ;;

   gcc)
      CFLAGS="$CFLAGS -Werror"
   ;;
esac

[ "$DEBUG" ] && CONFIGURE_FLAGS=$DEBUG_FLAGS || CONFIGURE_FLAGS=$RELEASE_FLAGS
[ "$VALGRIND" ] && TARGET="valgrind" || TARGET="test"

if [ "$RELEASE" ]; then
   # Overwrite the git checkout with the packaged archive
   $TAR xf ../libbson.tar.gz -C . --strip-components=1
   CONFIGURE_SCRIPT="./configure"
fi

echo "CFLAGS: $CFLAGS"
echo "MARCH: $MARCH"
echo "OS: $OS"
echo "CC: $CC"
echo "DEBUG: $DEBUG"
echo "RELEASE: $RELEASE"
echo "VALGRIND: $VALGRIND"

CFLAGS="$CFLAGS" CC="$CC" $CONFIGURE_SCRIPT $CONFIGURE_FLAGS
make $TARGET TEST_ARGS="--no-fork -d"

