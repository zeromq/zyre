Format:         1.0
Source:         zyre
Version:        1.1.0-1
Binary:         zyre, libzyre1
Architecture:   any all
Maintainer:     John Doe <John.Doe@example.com>
Standards-Version: 3.9.5
Build-Depends: bison, debhelper (>= 8),
    pkg-config,
    automake,
    autoconf,
    libtool,
    libzmq4-dev,
    libczmq-dev,
    dh-autoreconf

Package-List:
 zyre dev net optional arch-any
 libzyre1 dev net optional arch-any

