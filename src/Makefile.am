lib_LTLIBRARIES += libbson-1.0.la

libbson_1_0_la_SOURCES =
libbson_1_0_la_LIBADD = \
	libbson.la \
	-lm
libbson_1_0_la_LDFLAGS = \
	$(OPTIMIZE_LDFLAGS) \
	$(COVERAGE_LDFLAGS) \
	-no-undefined \
	-rpath $(libdir)

include src/bson/Makefile.am
include src/jsonsl/Makefile.am

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(top_builddir)/src/libbson-1.0.pc

EXTRA_DIST += \
	src/libbson-1.0.pc.in
