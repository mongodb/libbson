noinst_LTLIBRARIES = libyajl.la

YAJL_H_FILES = \
	yajl/api/yajl_tree.h \
	yajl/api/yajl_parse.h \
	yajl/api/yajl_gen.h \
	yajl/api/yajl_common.h \
	yajl/api/yajl_version.h \
	yajl/yajl_parser.h \
	yajl/yajl_encode.h \
	yajl/yajl_buf.h \
	yajl/yajl_alloc.h \
	yajl/yajl_bytestack.h \
	yajl/yajl_lex.h

YAJL_C_FILES = \
	yajl/yajl_version.c \
	yajl/yajl.c \
	yajl/yajl_encode.c \
	yajl/yajl_lex.c \
	yajl/yajl_parser.c \
	yajl/yajl_buf.c \
	yajl/yajl_tree.c \
	yajl/yajl_alloc.c \
	yajl/yajl_gen.c

libyajl_la_SOURCES = $(YAJL_H_FILES) $(YAJL_C_FILES)
libyajl_la_CPPFLAGS = \
	-I$(top_srcdir)/bson \
	-I$(top_srcdir)/yajl \
	-DBSON_COMPILATION
