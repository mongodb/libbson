headerdir = $(prefix)/include/libbson-@BSON_API_VERSION@
header_DATA = $(INST_H_FILES)


lib_LTLIBRARIES = libbson-1.0.la


AM_CPPFLAGS =

if OS_SOLARIS
AM_CPPFLAGS += -DOS_SOLARIS
endif

if OS_LINUX
AM_CPPFLAGS += -DOS_LINUX
endif

if OS_WIN32
AM_CPPFLAGS += -DOS_WIN32
endif


INST_H_FILES = \
	bson/bson.h \
	bson/bson-atomic.h \
	bson/bson-compat.h \
	bson/bson-config.h \
	bson/bson-context.h \
	bson/bson-clock.h \
	bson/bson-endian.h \
	bson/bson-error.h \
	bson/bson-iter.h \
	bson/bson-json.h \
	bson/bson-keys.h \
	bson/bson-macros.h \
	bson/bson-md5.h \
	bson/bson-memory.h \
	bson/bson-oid.h \
	bson/bson-reader.h \
	bson/bson-stdint.h \
	bson/bson-stdint-win32.h \
	bson/bson-string.h \
	bson/bson-types.h \
	bson/bson-utf8.h \
	bson/bson-version.h \
	bson/bson-writer.h \
	bson/bcon.h


NOINST_H_FILES = \
	bson/b64_ntop.h \
	bson/bson-context-private.h \
	bson/bson-thread-private.h \
	bson/bson-private.h


libbson_1_0_la_SOURCES = \
	$(INST_H_FILES) \
	$(NOINST_H_FILES) \
	bson/bson.c \
	bson/bson-context.c \
	bson/bson-clock.c \
	bson/bson-error.c \
	bson/bson-iter.c \
	bson/bson-json.c \
	bson/bson-keys.c \
	bson/bson-md5.c \
	bson/bson-memory.c \
	bson/bson-oid.c \
	bson/bson-reader.c \
	bson/bson-string.c \
	bson/bson-utf8.c \
	bson/bson-writer.c \
	bson/bcon.c


libbson_1_0_la_CPPFLAGS = \
	-DBSON_COMPILATION \
	$(BSON_DEBUG_CFLAGS) \
	-I$(top_srcdir)/bson \
	-I$(top_srcdir)/yajl \
	$(MAINTAINER_CFLAGS)


libbson_1_0_la_LIBADD = \
	$(PTHREAD_LIB) \
	libyajl.la


libbson_1_0_la_LDFLAGS = \
	$(BSON_LT_LDFLAGS) \
	-no-undefined \
	-export-symbols-regex "^(bson_|bcon_|BCON_MAGIC|BCONE_MAGIC)" \
	-rpath $(libdir)


DISTCLEANFILES += \
	bson/bson-stdint.h


EXTRA_DIST += \
	bson/libbson.symbols \
	bson/bson-config.h.in \
	bson/bson-version.h.in
