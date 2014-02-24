dnl **************************************************************************
dnl Versioning Macros
dnl **************************************************************************

m4_define([bson_major_version], [0])
m4_define([bson_minor_version], [4])
m4_define([bson_micro_version], [3])
m4_define([bson_version], [bson_major_version.bson_minor_version.bson_micro_version])

# bump up by 1 for every micro release with no API changes, otherwise
# set to 0. after release, bump up by 1
m4_define([bson_interface_age], [1])
m4_define([bson_binary_age], [m4_eval(100 * bson_minor_version + bson_micro_version)])

m4_define([lt_current], [m4_eval(100 * bson_minor_version + bson_micro_version - bson_interface_age)])
m4_define([lt_revision], [bson_interface_age])
m4_define([lt_age], [m4_eval(bson_binary_age - bson_interface_age)])


dnl **************************************************************************
dnl Initialize Automake
dnl **************************************************************************
AC_PREREQ([2.61])
AC_INIT([libbson],[bson_version],[https://github.com/mongodb/libbson/issues])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([build/m4])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES],
	 [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])
AS_AM_REALLY_SILENT

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

AM_INIT_AUTOMAKE([1.10])
AM_PROG_CC_C_O


dnl **************************************************************************
dnl Versioning Definitions
dnl **************************************************************************
BSON_MAJOR_VERSION=bson_major_version
BSON_MINOR_VERSION=bson_minor_version
BSON_MICRO_VERSION=bson_micro_version
BSON_API_VERSION=1.0
BSON_VERSION=bson_version
AC_SUBST(BSON_MAJOR_VERSION)
AC_SUBST(BSON_MINOR_VERSION)
AC_SUBST(BSON_MICRO_VERSION)
AC_SUBST(BSON_API_VERSION)
AC_SUBST(BSON_VERSION)

BSON_LT_CURRENT=lt_current
BSON_LT_REVISION=lt_revision
BSON_LT_AGE=lt_age
BSON_LT_VERSION="$BSON_LT_CURRENT:$BSON_LT_REVISION:$BSON_LT_AGE"
BSON_LT_LDFLAGS="-version-info $BSON_LT_VERSION"


dnl **************************************************************************
dnl Platform Checks
dnl **************************************************************************
platform_linux=no
platform_solaris=no
platform_win32=no

BSON_OS=1

AC_CANONICAL_HOST
AC_MSG_CHECKING([if building for some Win32 platform])
AS_CASE([$host],
	[*-*-mingw*],
	[
		BSON_LT_LDFLAGS="$BSON_LT_LDFLAGS -no-undefined -lws2_32"
                CFLAGS="$CFLAGS -D_WIN32_WINNT=0x0600"
		platform_win32=yes
		BSON_OS=2
	],

	[*-*-linux*|*-*-cygwin*],
	[
		platform_linux=yes
	],

	[*-*-solaris*],
	[
		platform_solaris=yes
	],

	[]
)
AC_MSG_RESULT([$platform_win32])

AM_CONDITIONAL(OS_LINUX,   [test "$platform_linux" = "yes"])
AM_CONDITIONAL(OS_SOLARIS, [test "$platform_solaris" = "yes"])
AM_CONDITIONAL(OS_WIN32,   [test "$platform_win32" = "yes"])

AC_SUBST([BSON_OS])


dnl **************************************************************************
dnl Symbolic Linking options
dnl **************************************************************************
# Check for -Bsymbolic-functions to avoid intra-library PLT jumps
AC_ARG_ENABLE([Bsymbolic],
              [AS_HELP_STRING([--disable-Bsymbolic],
                              [Avoid linking with -Bsymbolic])],
              [],
              [
                saved_LDFLAGS="${LDFLAGS}"
                AC_MSG_CHECKING([for -Bsymbolic-functions linker flag])
                LDFLAGS=-Wl,-Bsymbolic-functions
                AC_TRY_LINK([], [int main (void) { return 0; }],
                            [
                              AC_MSG_RESULT([yes])
                              enable_Bsymbolic=yes
                            ],
                            [
                              AC_MSG_RESULT([no])
                              enable_Bsymbolic=no
                            ])
                LDFLAGS="${saved_LDFLAGS}"
              ])

AS_IF([test "x$enable_Bsymbolic" = "xyes"], [BSON_LINK_FLAGS=-Wl[,]-Bsymbolic-functions])
BSON_LT_LDFLAGS="$BSON_LT_LDFLAGS $BSON_LINK_FLAGS"

AC_SUBST(BSON_LT_LDFLAGS)


dnl **************************************************************************
dnl Check for Required Headers
dnl **************************************************************************
AC_CREATE_STDINT_H([bson/bson-stdint.h])

# stdbool.h
AC_HEADER_STDBOOL
BSON_HAVE_STDBOOL_H=0
if test "$ac_cv_header_stdbool_h" = "yes" ; then
	BSON_HAVE_STDBOOL_H=1
fi
AC_SUBST([BSON_HAVE_STDBOOL_H])


dnl **************************************************************************
dnl Check for Python Support
dnl **************************************************************************
AC_ARG_ENABLE(
	[python],
	[AC_HELP_STRING([--enable-python=@<:no/yes@:>@],
			[Enable support for experimental python bindings. @<:@default=no@:>@])],
	[], [
		enable_python=no
		ax_python_header=no
	]
)
AS_IF([test "x$enable_python" = "xyes"], [
	AX_PYTHON
])
AM_CONDITIONAL([HAVE_PYTHON], [test "x$ax_python_header" != "xno"])


dnl **************************************************************************
dnl Check for necessary libraries.
dnl **************************************************************************
AC_CHECK_FUNCS(strnlen snprintf)

AC_SEARCH_LIBS([clock_gettime], [rt], [
	AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Have clock_gettime])
])
if test "$ac_cv_search_clock_gettime" = "-lrt"; then
	LDFLAGS="$LDFLAGS -lrt"
fi


dnl **************************************************************************
dnl Check if pthread_mutex synchronisation needed
dnl **************************************************************************
if test "$platform_solaris" = "yes"; then
	pthread_flag="-pthreads"
else
	pthread_flag="-pthread"
fi

AX_PTHREAD([
   if test "$platform_win32" = "no"; then
	PTHREAD_LIB="-lpthread"
	AC_TRY_LINK([#include <stdint.h>],
	   [uint32_t seq = __sync_fetch_and_add_4(&seq, 1);],
	   ,
	   AC_DEFINE([WITH_OID32_PT], [],
		     [pthread locking for bson_context_get_oid_seq32_threadsafe()])
	   PTHREAD_CFLAGS="$pthread_flag"
	   PTHREAD_LDFLAGS="$pthread_flag"
	)
	AC_TRY_LINK([#include <stdint.h>],
	   [uint64_t seq = __sync_fetch_and_add_8(&seq, 1);],
	   ,
	   AC_DEFINE([WITH_OID64_PT], [],
		     [pthread locking for bson_context_get_oid_seq64_threadsafe()])
	   PTHREAD_CFLAGS="$pthread_flag"
	   PTHREAD_LDFLAGS="$pthread_flag"
	)
	CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
	LDFLAGS="$LDFLAGS $PTHREAD_LDFLAGS"
	AC_SUBST(PTHREAD_LIB)
	enable_pthreads=yes
     else
	enable_pthreads=no
     fi
],[
	enable_pthreads=no
])
AM_CONDITIONAL(HAVE_PTHREADS, test "x$enable_pthreads" = "xyes")


dnl **************************************************************************
dnl Check Host Endianness
dnl **************************************************************************
AC_C_BIGENDIAN
if test "x$ac_cv_c_bigendian" = "xyes"; then
	BSON_BYTE_ORDER=4321
	host_endianness="Big Endian"
else
	BSON_BYTE_ORDER=1234
	host_endianness="Little Endian"
fi
CPPFLAGS="$CPPFLAGS -DBSON_BYTE_ORDER=$BSON_BYTE_ORDER"
AC_SUBST([BSON_BYTE_ORDER])


dnl **************************************************************************
dnl Optionally check for Asciidoc and xmlto for documentation
dnl **************************************************************************
AC_ARG_VAR([XMLTO], [Path to xmlto command])
AC_PATH_PROG([XMLTO], [xmlto])
AC_ARG_VAR([ASCIIDOC], [Path to asciidoc command])
AC_PATH_PROG([ASCIIDOC], [asciidoc])

BSON_CHECK_DOC_BUILD


dnl **************************************************************************
dnl Enable extra debugging options
dnl **************************************************************************

m4_define([debug_default], [m4_if(m4_eval(bson_minor_version % 2), [1], [yes], [minimum])])
AC_ARG_ENABLE([debug],
	      [AS_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
	      		      [turn on debugging @<:@default=debug_default@:>@])],
	      		      [],
              [enable_debug=debug_default])
AS_CASE([$enable_debug],
	[yes], [
	 	BSON_DEBUG_CFLAGS="-DBSON_ENABLE_DEBUG"
	],
	[minimum], [
		BSON_DEBUG_CFLAGS="-DBSON_ENABLE_DEBUG -DBSON_DISABLE_ASSERT"
	],
	[no], [
	 	BSON_DEBUG_CFLAGS="-DBSON_DISABLE_ASSERT -DBSON_DISABLE_CHECKS"
	],
	[AC_MSG_ERROR([Unknown argument to --enable-debug])])
AC_SUBST([BSON_DEBUG_CFLAGS])


dnl **************************************************************************
dnl Enable strict compiler flags
dnl **************************************************************************

# use strict compiler flags only on development releases
m4_define([maintainer_flags_default], [m4_if(m4_eval(bson_minor_version % 2), [1], [yes], [no])])
AC_ARG_ENABLE([maintainer-flags],
              [AS_HELP_STRING([--enable-maintainer-flags=@<:@no/yes@:>@],
                              [Use strict compiler flags @<:@default=]maintainer_flags_default[@:>@])],
              [],
              [enable_maintainer_flags=maintainer_flags_default])

AS_IF([test "x$enable_maintainer_flags" = "xyes" && test "x$GCC" = "xyes"],
      [AS_COMPILER_FLAGS([MAINTAINER_CFLAGS],
                         ["-Wall
                           -Waggregate-return
                           -Wcast-align
                           -Wdeclaration-after-statement
                           -Wempty-body
                           -Wformat
                           -Wformat-nonliteral
                           -Wformat-security
                           -Winit-self
                           -Winline
                           -Wmissing-include-dirs
                           -Wno-strict-aliasing
                           -Wno-uninitialized
		           -Wredundant-decls
                           -Wreturn-type
                           -Wshadow
                           -Wswitch-default
                           -Wswitch-enum
                           -Wundef
                           -Wuninitialized
                          "])]
)
MAINTAINER_CFLAGS="${MAINTAINER_CFLAGS#*  }"
AC_SUBST([MAINTAINER_CFLAGS])


dnl **************************************************************************
dnl Pthread workarounds
dnl **************************************************************************

# The following is borrowed from the guile configure script.
#
# On past versions of Solaris, believe 8 through 10 at least, you
# had to write "pthread_once_t foo = { PTHREAD_ONCE_INIT };".
# This is contrary to POSIX:
# http://www.opengroup.org/onlinepubs/000095399/functions/pthread_once.html
# Check here if this style is required.
#
# glibc (2.3.6 at least) works both with or without braces, so the
# test checks whether it works without.
#

AC_SUBST(NEED_BRACES_ON_PTHREAD_ONCE_INIT, 0)
AC_CACHE_CHECK([whether PTHREAD_ONCE_INIT needs braces],
  bson_cv_need_braces_on_pthread_once_init,
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <pthread.h>
     pthread_once_t foo = PTHREAD_ONCE_INIT;]])],
    [bson_cv_need_braces_on_pthread_once_init=no],
    [bson_cv_need_braces_on_pthread_once_init=yes])])
if test "$bson_cv_need_braces_on_pthread_once_init" = yes; then
  CFLAGS="$CFLAGS -D_PTHREAD_ONCE_INIT_NEEDS_BRACES"
fi


dnl **************************************************************************
dnl Output
dnl **************************************************************************
AC_CONFIG_FILES([
	Makefile
	bson/bson-version.h
	bson/bson-config.h
	bson/libbson-1.0.pc
])
AC_OUTPUT


if test $(( ${BSON_MINOR_VERSION} % 2 )) -eq 1; then
cat << EOF
 *** IMPORTANT *** 

 This is an unstable version of libbson.
 It is for test purposes only.

 Please, DO NOT use it in a production environment.
 It will probably crash and you will lose your data.

 Additionally, the API/ABI may change during the course
 of development.

 Thanks,

   The libbson team.

 *** END OF WARNING ***

EOF
fi


echo ""
echo "libbson-$VERSION"
echo ""
echo "  Prefix ....................: ${prefix}"
echo "  Endianness ................: ${host_endianness}"
echo "  Debug Level ...............: ${enable_debug}"
echo "  CFLAGS ....................: ${CFLAGS}"
echo "  Maintainer ................: ${enable_maintainer_flags}"
echo ""
echo "Experimental Bindings"
echo ""
echo "  Python ....................: ${ax_python_header}"
echo ""
