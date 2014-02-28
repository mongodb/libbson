if ENABLE_PYTHON

pymoduledir = $(libdir)/python2.7/site-packages

pymodule_LTLIBRARIES = cbson.la

cbson_la_SOURCES = \
	bindings/python/cbson.c \
	bindings/python/cbson-dbref.c \
	bindings/python/cbson-dbref.h \
	bindings/python/cbson-oid.c \
	bindings/python/cbson-oid.h \
	bindings/python/cbson-util.c \
	bindings/python/cbson-util.h \
	bindings/python/time64.c \
	bindings/python/time64.h \
	bindings/python/time64_config.h \
	bindings/python/time64_limits.h

cbson_la_LIBADD = \
	libbson-1.0.la \
	-l$(PYTHON_LIB)

cbson_la_LDFLAGS = -module

cbson_la_CPPFLAGS = -I$(PYTHON_INCLUDE_DIR)

endif
