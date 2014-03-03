lib_LTLIBRARIES += libbson-1.0.la

BSON_INST_H_FILES =
BSON_NOINST_H_FILES =

libbson_1_0_la_CPPFLAGS = \
	-DBSON_COMPILATION \
	-I$(libbson_srcdir)/src \
	-I$(libbson_srcdir)/src/bson \
	-I$(libbson_srcdir)/src/yajl \
	-I$(libbson_builddir)/src/bson

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

bsonpkgconfigdir = $(libdir)/pkgconfig
bsonpkgconfig_DATA = $(libbson_builddir)/src/libbson-1.0.pc

EXTRA_DIST += \
	$(libbson_srcdir)/src/libbson.symbols \
	$(libbson_srcdir)/src/libbson-1.0.pc.in
