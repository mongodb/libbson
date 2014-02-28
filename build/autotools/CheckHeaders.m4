AC_HEADER_STDBOOL
if test "$ac_cv_header_stdbool_h" = "yes"; then
	AC_DEFINE([BSON_HAVE_STDBOOL_H], 1, [libbson will use stdbool.h])
else
	AC_DEFINE([BSON_HAVE_STDBOOL_H], 0, [libbson will use bson-stdbool.h])
fi
AC_SUBST([BSON_HAVE_STDBOOL_H])

AC_CREATE_STDINT_H([src/bson/bson-stdint.h])
