EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =

AM_CPPFLAGS = \
	$(BSON_DEBUG_CFLAGS) \
	$(MAINGAINER_CFLAGS) \
	-D_BSD_SOURCE \
	-D_GNUC_SOURCE

include src/Makefile.am
include tests/Makefile.am
include examples/Makefile.am
include doc/Makefile.am

ACLOCAL_AMFLAGS = -I build/autotools/m4 ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-silent-rules

bsondocdir = ${docdir}
bsondoc_DATA =
bsondoc_DATA += COPYING
bsondoc_DATA += NEWS
bsondoc_DATA += README

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
	@if test -d "$(srcdir)/.git"; then                              \
          (cd "$(srcdir)" &&                                            \
	   $(top_srcdir)/missing --run git log --raw|grep "^Author: "|sort|uniq -c) > AUTHORS.tmp \
           && mv -f AUTHORS.tmp $(top_distdir)/AUTHORS                  \
	fi

uninstall-local:
	-rm -r $(bsondocdir)
