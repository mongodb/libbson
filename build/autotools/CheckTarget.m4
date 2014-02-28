AC_CANONICAL_SYSTEM

enable_crosscompile=no
if test "x$host" != "x$target"; then
    enable_crosscompile=yes
    case "$target" in
        arm*-darwin*)
            ;;
        powerpc64-*-linux-gnu)
            ;;
        arm*-linux-*)
            ;;
        *)
            AC_MSG_ERROR([Cross compiling is not supported for target $target])
            ;;
    esac
fi
