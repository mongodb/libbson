# Enable Link-Time-Optimization
OPTIMIZE_CFLAGS=""
OPTIMIZE_LDFLAGS=""
AS_IF([test "$enable_optimizations" != "no"], [
    check_cc_cxx_flag([-flto], [OPTIMIZE_CFLAGS="$OPTIMIZE_CFLAGS -flto"])
    check_link_flag([-flto], [OPTIMIZE_LDFLAGS="$OPTIMIZE_LDFLAGS -flto"])
    check_link_flag([-Bsymbolic], [OPTIMIZE_LDFLAGS="$OPTIMIZE_LDFLAGS -Wl,-Bsymbolic"])
])
