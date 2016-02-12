Format:         1.0
Source:         zyre
Version:        1.1.0-1
Binary:         libzyre1, zyre-dev
Architecture:   any all
Maintainer:     John Doe <John.Doe@example.com>
Standards-Version: 3.9.5
Build-Depends: bison, debhelper (>= 8),
    pkg-config,
    automake,
    autoconf,
    libtool,
    libzmq4-dev,
    uuid-dev,
    libczmq-dev,
    dh-autoreconf

Package-List:
 libzyre1 deb net optional arch=any
 zyre-dev deb libdevel optional arch=any

