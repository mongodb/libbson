AC_LANG_PUSH([C])
AC_MSG_CHECKING([for __sync_add_and_fetch_8])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdint.h>]],
                                [[int64_t v; __sync_add_and_fetch_8 (&v, (int64_t)10);]])],
               [AC_MSG_RESULT(yes)
                have_sync_add_and_fetch_8=yes],
               [AC_MSG_RESULT(no)
                have_sync_add_and_fetch_8=no])
AS_IF([test "$have_sync_add_and_fetch_8" = "yes"],
      [AC_SUBST(BSON_HAVE_ATOMIC_64_ADD_AND_FETCH, 1)],
      [AC_SUBST(BSON_HAVE_ATOMIC_64_ADD_AND_FETCH, 0)])
AC_LANG_POP([C])
