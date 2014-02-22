EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =

include yajl/Makefile.include
include bson/Makefile.include
include tests/Makefile.include
include examples/Makefile.include
include doc/Makefile.include

if HAVE_PYTHON
include bindings/python/Makefile.include
endif

ACLOCAL_AMFLAGS = -I build/m4 ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-silent-rules

bsondocdir = ${docdir}
bsondoc_DATA =
bsondoc_DATA += COPYING
bsondoc_DATA += NEWS
bsondoc_DATA += README

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =
pkgconfig_DATA += $(top_builddir)/bson/libbson-1.0.pc

EXTRA_DIST += $(bsondoc_DATA)

dist-hook:
	@if test -d "$(srcdir)/.git"; then                              \
          (cd "$(srcdir)" &&                                            \
           $(top_srcdir)/missing --run git log --stat ) > ChangeLog.tmp \
           && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog              \
           || (rm -f ChangeLog.tmp;                                     \
               echo Failed to generate ChangeLog >&2);                  \
	else                                                            \
	  echo A git checkout is required to generate a ChangeLog >&2;  \
	fi

uninstall-local:
	-rm -r $(bsondocdir)
