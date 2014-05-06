Name:           libbson
Version:        0.6.9
Release:        1%{?dist}
Summary:        BSON library

License:        ASL 2.0
URL:            https://github.com/mongodb/libbson
Source0:        https://github.com/mongodb/libbson/releases/download/0.6.9/libbson-0.6.9.tar.gz
BuildRequires:  automake

%description
libbson is a library providing useful routines related to 
building, parsing, and iterating BSON documents. It is a 
useful base for those wanting to write high-performance 
C extensions to higher level languages such as Python, 
Ruby, or Perl.

%package        devel
Summary:        Development files for libbson
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%prep
%setup -q -n %{name}-%{version}
automake 

%build
%configure --disable-static --disable-silent-rules --enable-debug-symbols --enable-hardening --docdir=%{_pkgdocdir}
make %{?_smp_mflags}

%check
make check

%install
%make_install
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%doc COPYING README NEWS
%{_libdir}/*.so.*

%files devel
%dir %{_includedir}/%{name}-1.0
%{_includedir}/%{name}-1.0/*.h
%{_libdir}/%{name}-1.0.so
%{_libdir}/pkgconfig/%{name}-1.0.pc
%{_prefix}/share/man/man3/*
%{_prefix}/share/man/man7/*

%changelog
* Tue May 06 2014 Christian Hergert <christian.hergert@mongodb.com> - 0.6.9-1
- Update to 0.6.9 from git.
- Enable debug symbols
- Enable autoharden extensions

* Fri Aug 30 2013 Mike Manilone <crtmike@gmx.us> - 0.2.0-2
- Fix docdir.
- Do not use silent rules.

* Wed Aug 28 2013 Mike Manilone <crtmike@gmx.us> - 0.2.0-1
- Update to 0.2.0.

* Tue Aug 13 2013 Mike Manilone <crtmike@gmx.us> - 0.1.10-7
- Debug level is changed to minimum.
- Disable python package.
- Update (affecting Pidora).

* Tue Aug 13 2013 Mike Manilone <crtmike@gmx.us> - 0.1.10-6
- Add 'check' section.

* Mon Aug 12 2013 Mike Manilone <crtmike@gmx.us> - 0.1.10-5
- Do not 'provide' private libraries.

* Mon Aug 12 2013 Mike Manilone <crtmike@gmx.us> - 0.1.10-4
- Update.

* Mon Aug 12 2013 Mike Manilone <crtmike@gmx.us> - 0.1.10-3
- Fix many problems.

* Mon Aug 12 2013 Mike Manilone <crtmike@gmx.us> - 0.1.10-2
- Make -devel subpackage a noarch package
- Add a patch to make it pass the compilation

* Mon Aug 12 2013 Mike Manilone <crtmike@gmx.us> - 0.1.10-1
- Initial package
