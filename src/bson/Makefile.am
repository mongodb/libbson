bsonheaderdir = $(prefix)/include/libbson-@BSON_API_VERSION@
bsonheader_DATA = $(BSON_INST_H_FILES)

BSON_INST_H_FILES += \
	$(libbson_srcdir)/src/bson/bcon.h \
	$(libbson_srcdir)/src/bson/bson.h \
	$(libbson_srcdir)/src/bson/bson-atomic.h \
	$(libbson_srcdir)/src/bson/bson-clock.h \
	$(libbson_srcdir)/src/bson/bson-compat.h \
	$(libbson_srcdir)/src/bson/bson-context.h \
	$(libbson_srcdir)/src/bson/bson-endian.h \
	$(libbson_srcdir)/src/bson/bson-error.h \
	$(libbson_srcdir)/src/bson/bson-iter.h \
	$(libbson_srcdir)/src/bson/bson-json.h \
	$(libbson_srcdir)/src/bson/bson-keys.h \
	$(libbson_srcdir)/src/bson/bson-macros.h \
	$(libbson_srcdir)/src/bson/bson-md5.h \
	$(libbson_srcdir)/src/bson/bson-memory.h \
	$(libbson_srcdir)/src/bson/bson-oid.h \
	$(libbson_srcdir)/src/bson/bson-reader.h \
	$(libbson_srcdir)/src/bson/bson-string.h \
	$(libbson_srcdir)/src/bson/bson-types.h \
	$(libbson_srcdir)/src/bson/bson-utf8.h \
	$(libbson_srcdir)/src/bson/bson-writer.h

if OS_WIN32
BSON_INST_H_FILES += $(libbson_srcdir)/src/bson/bson-stdint-win32.h
endif

BSON_NOINST_H_FILES += \
	$(libbson_srcdir)/src/bson/b64_ntop.h \
	$(libbson_srcdir)/src/bson/b64_pton.h \
	$(libbson_srcdir)/src/bson/bson-private.h \
	$(libbson_srcdir)/src/bson/bson-context-private.h \
	$(libbson_srcdir)/src/bson/bson-thread-private.h


libbson_1_0_la_SOURCES += \
	$(libbson_srcdir)/src/bson/bcon.c \
	$(libbson_srcdir)/src/bson/bson.c \
	$(libbson_srcdir)/src/bson/bson-clock.c \
	$(libbson_srcdir)/src/bson/bson-context.c \
	$(libbson_srcdir)/src/bson/bson-error.c \
	$(libbson_srcdir)/src/bson/bson-iter.c \
	$(libbson_srcdir)/src/bson/bson-json.c \
	$(libbson_srcdir)/src/bson/bson-keys.c \
	$(libbson_srcdir)/src/bson/bson-md5.c \
	$(libbson_srcdir)/src/bson/bson-memory.c \
	$(libbson_srcdir)/src/bson/bson-oid.c \
	$(libbson_srcdir)/src/bson/bson-reader.c \
	$(libbson_srcdir)/src/bson/bson-string.c \
	$(libbson_srcdir)/src/bson/bson-utf8.c \
	$(libbson_srcdir)/src/bson/bson-writer.c

EXTRA_DIST += \
	$(libbson_srcdir)/src/bson/bson-config.h.in \
	$(libbson_srcdir)/src/bson/bson-version.h.in

DISTCLEANFILES += \
	$(libbson_builddir)/src/bson/bson-stdint.h
