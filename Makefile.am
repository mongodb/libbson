EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

bin_PROGRAMS = 
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
noinst_PROGRAMS =

include src/Makefile.am
include doc/Makefile.am
if ENABLE_MAN_PAGES
include doc/man/Makefile.am
endif
if ENABLE_HTML_DOCS
include doc/html/Makefile.am
endif
if ENABLE_TESTS
include tests/Makefile.am
endif

if ENABLE_EXAMPLES
include examples/Makefile.am
endif

# for various extra dist
include build/cmake/Makefile.am

ACLOCAL_AMFLAGS = -I build/autotools/m4 ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-maintainer-flags --enable-silent-rules --enable-optimizations --enable-html-docs --enable-man-pages

bsondocdir = ${docdir}
bsondoc_DATA = \
	COPYING \
	NEWS \
	README \
	THIRD_PARTY_NOTICES

EXTRA_DIST += $(bsondoc_DATA)

EXTRA_DIST += CMakeLists.txt \
    VERSION_CURRENT \
    VERSION_RELEASED

dist-hook:
	@if test -d "$(srcdir)/.git"; then                              \
          (cd "$(srcdir)" &&                                            \
           $(top_srcdir)/build/autotools/missing --run git log --stat ) > ChangeLog.tmp \
           && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog              \
           || (rm -f ChangeLog.tmp;                                     \
               echo Failed to generate ChangeLog >&2);                  \
	else                                                            \
	  echo A git checkout is required to generate a ChangeLog >&2;  \
	fi

uninstall-local:
	-rmdir $(bsondocdir)/doc $(bsondocdir)/html
	-rm -r $(bsondocdir)
