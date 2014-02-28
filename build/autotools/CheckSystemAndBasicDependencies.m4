AC_PATH_PROG(PERL, perl)
if test -z "$PERL"; then
    AC_MSG_ERROR([You need 'perl' to compile libbson])
fi

AC_PATH_PROG(MV, mv)
if test -z "$MV"; then
    AC_MSG_ERROR([You need 'mv' to compile libbson])
fi

AC_PATH_PROG(GREP, grep)
if test -z "$GREP"; then
    AC_MSG_ERROR([You need 'grep' to compile libbson])
fi


# If CFLAGS and CXXFLAGS are unset, default to empty.
# This is to tell automake not to include '-g' if C{XX,}FLAGS is not set.
# For more info - http://www.gnu.org/software/automake/manual/autoconf.html#C_002b_002b-Compiler
if test -z "$CXXFLAGS"; then
    CXXFLAGS=""
fi
if test -z "$CFLAGS"; then
    CFLAGS=""
fi

AC_PROG_CC
AC_PROG_INSTALL

# Check that an appropriate C compiler is available.
c_compiler="unknown"
AC_LANG_PUSH([C])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#if !(defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 4)
#error Not a supported GCC compiler
#endif
])], [c_compiler="gcc"], [])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#if !(defined(__clang__) && __clang_major__ >= 3 && __clang_minor__ >= 3)
#error Not a supported Clang compiler
#endif
])], [c_compiler="clang"], [])
AC_LANG_POP([C])

if test "$c_compiler" = "unknown"; then
    AC_MSG_ERROR([Compiler GCC >= 4.4 or Clang >= 3.3 is required for C compilation])
fi


# C99 Headers
AC_HEADER_STDBOOL
if test "$ac_cv_header_stdbool_h" = "yes"; then
	AC_DEFINE([BSON_HAVE_STDBOOL_H], 1, [libbson will use stdbool.h])
else
	AC_DEFINE([BSON_HAVE_STDBOOL_H], 0, [libbson will use bson-stdbool.h])
fi

AC_CREATE_STDINT_H([bson/bson-stdint.h])
