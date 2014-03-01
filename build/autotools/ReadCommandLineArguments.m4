AC_MSG_CHECKING([whether to do a debug build])
AC_ARG_ENABLE(debug, 
    AC_HELP_STRING([--enable-debug], [turn on debugging [default=no]]),
    [],[enable_debug="no"])
AC_MSG_RESULT([$enable_debug])

AC_MSG_CHECKING([whether to enable optimized builds])
AC_ARG_ENABLE(optimizations, 
    AC_HELP_STRING([--enable-optimizations], [turn on build-time optimizations [default=yes]]),
    [enable_optimizations=$enableval],
    [
        if test "$enable_debug" = "yes"; then
            enable_optimizations="no";
        else
            enable_optimizations="yes";
        fi
    ])
AC_MSG_RESULT([$enable_optimizations])

AC_MSG_CHECKING([whether to enable code coverage support])
AC_ARG_ENABLE(coverage,
    AC_HELP_STRING([--enable-coverage], [enable code coverage support [default=no]]),
    [],
    [enable_coverage="no"])
AC_MSG_RESULT([$enable_coverage])

AC_MSG_CHECKING([whether to enable debug symbols])
AC_ARG_ENABLE(debug_symbols,
    AC_HELP_STRING([--enable-debug-symbols=yes|no|min|full], [enable debug symbols default=no, default=yes for debug builds]),
    [
        case "$enable_debug_symbols" in
            yes) enable_debug_symbols="full" ;;
            no|min|full) ;;
            *) AC_MSG_ERROR([Invalid debug symbols option: must be yes, no, min or full.]) ;;
        esac
    ],
    [
         if test "$enable_debug" = "yes"; then
             enable_debug_symbols="yes";
         else
             enable_debug_symbols="no";
         fi
    ])
AC_MSG_RESULT([$enable_debug_symbols])

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
