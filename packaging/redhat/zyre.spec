#
#    zyre - an open-source framework for proximity-based P2P apps
#
#    Copyright (c) the Contributors as noted in the AUTHORS file.           
#                                                                           
#    This file is part of Zyre, an open-source framework for proximity-based
#    peer-to-peer applications -- See http://zyre.org.                      
#                                                                           
#    This Source Code Form is subject to the terms of the Mozilla Public    
#    License, v. 2.0. If a copy of the MPL was not distributed with this    
#    file, You can obtain one at http://mozilla.org/MPL/2.0/.               
#

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif
Name:           zyre
Version:        2.0.0
Release:        1
Summary:        an open-source framework for proximity-based p2p apps
License:        MIT
URL:            http://github.com/zeromq/zyre
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  xmlto
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
zyre an open-source framework for proximity-based p2p apps.

%package -n libzyre2
Group:          System/Libraries
Summary:        an open-source framework for proximity-based p2p apps shared library

%description -n libzyre2
This package contains shared library for zyre: an open-source framework for proximity-based p2p apps

%post -n libzyre2 -p /sbin/ldconfig
%postun -n libzyre2 -p /sbin/ldconfig

%files -n libzyre2
%defattr(-,root,root)
%{_libdir}/libzyre.so.*

%package devel
Summary:        an open-source framework for proximity-based p2p apps
Group:          System/Libraries
Requires:       libzyre2 = %{version}
Requires:       zeromq-devel
Requires:       czmq-devel

%description devel
an open-source framework for proximity-based p2p apps development tools
This package contains development files for zyre: an open-source framework for proximity-based p2p apps

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libzyre.so
%{_libdir}/pkgconfig/libzyre.pc
%{_mandir}/man3/*
%{_mandir}/man7/*
%{_datadir}/zproject/
%{_datadir}/zproject/zyre/

%prep
%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS}
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%files
%defattr(-,root,root)
%doc README.md
%doc README.txt
%{_bindir}/zpinger
%{_mandir}/man1/zpinger*

%changelog
