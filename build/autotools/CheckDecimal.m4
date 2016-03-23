# If CFLAGS and CXXFLAGS are unset, default to empty.
# This is to tell automake not to include '-g' if C{XX,}FLAGS is not set.
# For more info - http://www.gnu.org/software/automake/manual/autoconf.html#C_002b_002b-Compiler
if test -z "$CXXFLAGS"; then
    CXXFLAGS=""
fi
if test -z "$CFLAGS"; then
    CFLAGS=""
fi

AC_SUBST(BSON_HAVE_DECIMAL128, 0)

AS_IF([test -n "$enable_decimal"],
   [
      AS_IF([test "$enable_decimal" = "yes"],
         [
            AC_SUBST(BSON_HAVE_DECIMAL128, 1)
         ])
   ],
   [
      AC_PROG_CC
      AC_PROG_CXX

      AC_LANG_PUSH([C])

      AC_CHECK_TYPE([_Decimal128],
         [enable_decimal='yes'],
         [enable_decimal='no'])

      AS_IF([test "$enable_decimal" = "yes"],
         [
            AC_MSG_CHECKING([if _Decimal128 is binary integer decimal format])

            AS_VAR_IF(enable_bigendian, ["yes"]
               [_bigendian_b=1],
               [_bigendian_b=0])

            # Check if the system is BID.
            AC_RUN_IFELSE([AC_LANG_PROGRAM([[
                  #include <string.h>
                  #include "src/bson/bson-stdint.h"

                  #if $_bigendian_b
                     #define HIGH  0
                     #define LOW   1
                  #else
                     #define HIGH  1
                     #define LOW   0
                  #endif
               ]], [[
                  _Decimal128 d = 55.DL;
                  _Decimal128 d2 = 100.DL;
                  uint64_t parts[2];
                  memcpy (parts, &d, sizeof (d));

                  if (parts[HIGH] != 0x3040000000000000ULL ||
                      parts[LOW] != 0x0000000000000037ULL) {
                     return 1;
                  }

                  if (d + d != 2 * d) {
                     return 1;
                  }

                  if (d != d) {
                     return 1;
                  }

                  if (d + 1 != 56.DL) {
                     return 1;
                  }

                  d2 -= 200.DL;
                  d2 *= 10.DL;

                  if (d2 != -1000.DL) {
                     return 1;
                  }

                  return 0;
               ]])],
               [
                  enable_decimal='yes'
                  AC_SUBST(BSON_HAVE_DECIMAL128, 1)
               ],
               [
                  enable_decimal='no'
                  AC_SUBST(BSON_HAVE_DECIMAL128, 0)
               ],
               [
                  enable_decimal='no'
                  AC_SUBST(BSON_HAVE_DECIMAL128, 0)
                  AC_MSG_WARN([Cannot check for BID support with cross compiling. Use --<enable/disable>-decimal-bid.])
               ]
            )

            AC_MSG_RESULT([$enable_decimal])

         ])

      AC_LANG_POP([C])
   ])

