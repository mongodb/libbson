AC_CHECK_FUNC(strnlen)
AC_CHECK_FUNC(snprintf)

# bracket pthread

# Check for clock_gettime and if it needs -lrt
AC_SEARCH_LIBS([clock_gettime], [rt], [
    AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Have clock_gettime])
])
if test "$ac_cv_search_clock_gettime" = "-lrt"; then
    LDFLAGS="$LDFLAGS -lrt"
fi


if test "$os_solaris" = "yes"; then
    pthread_flag="-pthreads"
else
    pthread_flag="-pthread"
fi

# Check if we need to use a mutex due to no atomics support.
AX_PTHREAD([
   if test "$os_win32" != "yes"; then
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
    AC_DEFINE(BSON_PTHREAD_ONCE_INIT_NEEDS_BRACES, 1,
              [PTHREAD_ONCE_INIT needs braces])
fi
AC_SUBST([BSON_PTHREAD_ONCE_INIT_NEEDS_BRACES])
