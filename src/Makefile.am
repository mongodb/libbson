lib_LTLIBRARIES = libbson-1.0.la

INST_H_FILES =
NOINST_H_FILES =

libbson_1_0_la_CPPFLAGS = \
	-DBSON_COMPILATION \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/bson \
	-I$(top_srcdir)/src/yajl \
	-I$(top_builddir)/src/bson

libbson_1_0_la_CFLAGS = \
	$(OPTIMIZE_CFLAGS)

libbson_1_0_la_SOURCES = \
	$(INST_H_FILES) \
	$(NOINST_H_FILES)

libbson_1_0_la_LDFLAGS = \
	$(OPTIMIZE_LDFLAGS) \
	$(BSON_LT_LDFLAGS) \
	-no-undefined \
	-export-symbols-regex "^(bson_|bcon_)" \
	-rpath $(libdir)

libbson_1_0_la_LIBADD =
if !OS_WIN32
libbson_1_0_la_LIBADD += $(PTHREAD_LIB)
else
libbson_1_0_la_LIBADD += -lws2_32
endif

if !ENABLE_DEBUG
libbson_1_0_la_CPPFLAGS += \
	-DBSON_DISABLE_ASSERT \
	-DBSON_DISABLE_CHECKS
endif

include src/bson/Makefile.am
include src/yajl/Makefile.am
include src/bindings/Makefile.am

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(top_builddir)/src/libbson-1.0.pc

EXTRA_DIST += \
	src/libbson.symbols \
	src/libbson-1.0.pc.in
