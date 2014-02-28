BSON_LT_CURRENT=lt_current
BSON_LT_REVISION=lt_revision
BSON_LT_AGE=lt_age
BSON_LT_VERSION="$BSON_LT_CURRENT:$BSON_LT_REVISION:$BSON_LT_AGE"
BSON_LT_LDFLAGS="-version-info $BSON_LT_VERSION"

if test -z "$AR_FLAGS"; then
    if "${AR:-ar}" -V | grep -q 'GNU ar'; then
        AR_FLAGS="cruT"
    else
        AR_FLAGS="cru"
    fi
fi
AC_SUBST([AR_FLAGS])

LT_PREREQ([2.2])

AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

