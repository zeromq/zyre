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

Name:           zyre
Version:        1.1.0
Release:        1
Summary:        an open-source framework for proximity-based p2p apps
License:        MIT
URL:            http://example.com/
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkg-config
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
zyre an open-source framework for proximity-based p2p apps.

%package -n libzyre1
Group:          System/Libraries
Summary:        an open-source framework for proximity-based p2p apps

%description -n libzyre1
zyre an open-source framework for proximity-based p2p apps.
This package contains shared library.

%post -n libzyre1 -p /sbin/ldconfig
%postun -n libzyre1 -p /sbin/ldconfig

%files -n libzyre1
%defattr(-,root,root)
%doc COPYING
%{_libdir}/libzyre.so.*

%package devel
Summary:        an open-source framework for proximity-based p2p apps
Group:          System/Libraries
Requires:       libzyre1 = %{version}
Requires:       zeromq-devel
Requires:       czmq-devel

%description devel
zyre an open-source framework for proximity-based p2p apps.
This package contains development files.

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libzyre.so
%{_libdir}/pkgconfig/zyre.pc

%prep
%setup -q

%build
sh autogen.sh
%{configure}
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%files
%defattr(-,root,root)
%doc README.md
%doc COPYING
%{_bindir}/perf_local
%{_bindir}/perf_remote
%{_bindir}/zpinger
%{_bindir}/ztester_beacon
%{_bindir}/ztester_gossip

%changelog
