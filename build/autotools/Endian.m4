AC_C_BIGENDIAN
if test "x$ac_cv_c_bigendian" = "xyes"; then
    AC_DEFINE([BSON_BYTE_ORDER], 4321, [libbson is compiling for big-endian.])
    enable_bigendian=yes
else
    AC_DEFINE([BSON_BYTE_ORDER], 1234, [libbson is compiling for little-endian.])
    enable_bigendian=no
fi
