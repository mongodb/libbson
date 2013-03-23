dnl **************************************************************************
dnl Versioning Macros
dnl **************************************************************************
m4_define([bson_major_version], [0])
m4_define([bson_minor_version], [1])
m4_define([bson_micro_version], [0])
m4_define([bson_version],
          [bson_major_version.bson_minor_version.bson_micro_version])
m4_define([bson_binary_age], [m4_eval(100 * bson_minor_version +
           bson_micro_version)])
m4_define([lt_current], [m4_eval(100 * bson_minor_version +
           bson_micro_version - bson_interface_age)])
m4_define([lt_revision], [bson_interface_age])
m4_define([lt_age], [m4_eval(bson_binary_age - bson_interface_age)])


dnl **************************************************************************
dnl Initialize Automake
dnl **************************************************************************
AC_PREREQ([2.64])
AC_INIT([bson],[bson_version],[https://github.com/chergert/libbson])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AS_AM_REALLY_SILENT

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

AM_INIT_AUTOMAKE([1.11.1 no-dist-gzip dist-xz -Wno-portability])


dnl **************************************************************************
dnl Versioning Definitions
dnl **************************************************************************
BSON_MAJOR_VERSION=bson_major_version
BSON_MINOR_VERSION=bson_minor_version
BSON_MICRO_VERSION=bson_micro_version
BSON_API_VERSION=1.0
BSON_VERSION=bson_version
AC_SUBST(BSON_MAJOR_VERSION)
AC_SUBST(BSON_MINOR_VERSION)
AC_SUBST(BSON_MICRO_VERSION)
AC_SUBST(BSON_API_VERSION)
AC_SUBST(BSON_VERSION)


dnl **************************************************************************
dnl Enable extra debugging options
dnl **************************************************************************
m4_define([debug_default],
	  [m4_if(m4_eval(bson_minor_version % 2), [1],
	  [yes], [minimum])])
AC_ARG_ENABLE([debug],
	      [AS_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
	      		      [turn on debugging @<:@default=debug_default@:>@])],
	      		      [],
	      		      [enable_debug=debug_default])
AS_CASE([$enable_debug],
	[yes], [
		CFLAGS="$CFLAGS -g"
		CFLAGS="$CFLAGS -Wall"
		CFLAGS="$CFLAGS -Werror"
		CFLAGS="$CFLAGS -Wold-style-definition"
		CFLAGS="$CFLAGS -Wdeclaration-after-statement"
		CFLAGS="$CFLAGS -Wredundant-decls"
		CFLAGS="$CFLAGS -Wmissing-noreturn"
		CFLAGS="$CFLAGS -Wcast-align"
		CFLAGS="$CFLAGS -Wwrite-strings"
		CFLAGS="$CFLAGS -Winline"
		CFLAGS="$CFLAGS -Wformat-nonliteral"
		CFLAGS="$CFLAGS -Wformat-security"
		CFLAGS="$CFLAGS -Wswitch-enum"
		CFLAGS="$CFLAGS -Wswitch-default"
		CFLAGS="$CFLAGS -Winit-self"
		CFLAGS="$CFLAGS -Wmissing-include-dirs"
		CFLAGS="$CFLAGS -Wundef"
		CFLAGS="$CFLAGS -Waggregate-return"
		CFLAGS="$CFLAGS -Wmissing-format-attribute"
		CFLAGS="$CFLAGS -Wnested-externs"
		CFLAGS="$CFLAGS -Wshadow"
	],
	[minimum], [
		CFLAGS="$CFLAGS -DG_DISABLE_CAST_CHECKS"
	],
	[no], [
		CFLAGS="$CFLAGS -DG_DISABLE_ASSERT"
		CFLAGS="$CFLAGS -DG_DISABLE_CHECKS"
		CFLAGS="$CFLAGS -DG_DISABLE_CAST_CHECKS"
	],
	[AC_MSG_ERROR([Unknown argument to --enable-debug])]
)


dnl **************************************************************************
dnl Output
dnl **************************************************************************
AC_CONFIG_FILES([
	Makefile
	libbson-1.0.pc
])
AC_OUTPUT


echo ""
echo "libbson-$VERSION"
echo ""
echo "  Prefix ....................: ${prefix}"
echo "  Debug Level ...............: ${enable_debug}"
echo "  CFLAGS ....................: ${CFLAGS}"
echo ""
