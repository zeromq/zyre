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

# build with python_cffi support enabled
%bcond_with python_cffi
%if %{with python_cffi}
%define py2_ver %(python2 -c "import sys; print ('%d.%d' % (sys.version_info.major, sys.version_info.minor))")
%endif

# build with python3_cffi support enabled
%bcond_with python3_cffi
%if %{with python3_cffi}
%define py3_ver %(python3 -c "import sys; print ('%d.%d' % (sys.version_info.major, sys.version_info.minor))")
%endif

Name:           zyre
Version:        2.0.1
Release:        1
Summary:        an open-source framework for proximity-based p2p apps
License:        MPLv2
URL:            https://github.com/zeromq/zyre
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
%if %{with python_cffi}
BuildRequires:  python-cffi
BuildRequires:  python-devel
BuildRequires:  python-setuptools
%endif
%if %{with python3_cffi}
BuildRequires:  python3-devel
BuildRequires:  python3-cffi
BuildRequires:  python3-setuptools
%endif
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
# Install api files into /usr/local/share/zproject
%dir %{_datadir}/zproject/
%dir %{_datadir}/zproject/zyre
%{_datadir}/zproject/zyre/*

%if %{with python_cffi}
%package -n python2-zyre-cffi
Group: Python
Summary: Python CFFI bindings for zyre
Requires: python >= %{py2_ver}.0, python < 3.0.0

%description -n python2-zyre-cffi
This package contains Python CFFI bindings for zyre

%files -n python2-zyre-cffi
%{_libdir}/python%{py2_ver}/site-packages/zyre_cffi/
%{_libdir}/python%{py2_ver}/site-packages/zyre_cffi-*-py%{py2_ver}.egg-info/
%endif

%if %{with python3_cffi}
%package -n python3-zyre-cffi
Group: Python
Summary: Python 3 CFFI bindings for zyre
Requires: python = %{py3_ver}

%description -n python3-zyre-cffi
This package contains Python 3 CFFI bindings for zyre

%files -n python3-zyre-cffi
%{_libdir}/python%{py3_ver}/site-packages/zyre_cffi/
%{_libdir}/python%{py3_ver}/site-packages/zyre_cffi-*-py%{py3_ver}.egg-info/
%endif

%prep
#FIXME: error:... did not worked for me
%if %{with python_cffi}
%if %{without drafts}
echo "FATAL: python_cffi not yet supported w/o drafts"
exit 1
%endif
%endif

%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS}
make %{_smp_mflags}
%if %{with python_cffi} || %{with python3_cffi}
# Problem: we need pkg-config points to built and not yet installed copy of zyre
# Solution: chicken-egg problem - let's make "fake" pkg-config file
sed -e "s@^libdir.*@libdir=.libs/@" \
    -e "s@^includedir.*@includedir=include/@" \
    src/libzyre.pc > bindings/python_cffi/libzyre.pc
cd bindings/python_cffi
# This avoids problem with "weird" character quoting between shell and python3
ln -sfr ../../include/ .
ln -sfr ../../src/.libs/ .
export PKG_CONFIG_PATH=`pwd`
%endif
%if %{with python_cffi}
python2 setup.py build
%endif
%if %{with python3_cffi}
python3 setup.py build
%endif

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%if %{with python_cffi} || %{with python3_cffi}
cd bindings/python_cffi
export PKG_CONFIG_PATH=`pwd`
%endif
%if %{with python_cffi}
python2 setup.py install --root=%{buildroot} --skip-build --prefix %{_prefix}
%endif
%if %{with python3_cffi}
python3 setup.py install --root=%{buildroot} --skip-build --prefix %{_prefix}
%endif

%files
%defattr(-,root,root)
%doc README.md
%doc README.txt
%{_bindir}/zpinger
%{_mandir}/man1/zpinger*

%changelog
