.set GIT=https://github.com/zeromq/zyre
.sub 0MQ=ØMQ

[![GitHub release](https://img.shields.io/github/release/zeromq/zyre.svg)](https://github.com/zeromq/zyre/releases)
[![OBS draft](https://img.shields.io/badge/OBS%20master-draft-yellow.svg)](http://software.opensuse.org/download.html?project=network%3Amessaging%3Azeromq%3Agit-draft&package=zyre)
[![OBS stable](https://img.shields.io/badge/OBS%20master-stable-yellow.svg)](http://software.opensuse.org/download.html?project=network%3Amessaging%3Azeromq%3Agit-stable&package=zyre)
<a target="_blank" href="http://webchat.freenode.net?channels=%23zeromq&uio=d4"><img src="https://cloud.githubusercontent.com/assets/493242/14886493/5c660ea2-0d51-11e6-8249-502e6c71e9f2.png" height = "20" /></a>
[![license](https://img.shields.io/badge/license-MPL%202.0-green.svg)](https://github.com/zeromq/zyre/blob/master/LICENSE)

# Zyre - Local Area Clustering for Peer-to-Peer Applications

| Linux & MacOSX | Windows  |
|:--------------:|:--------:|
|[![Build Status](https://travis-ci.org/zeromq/zyre.png?branch=master)](https://travis-ci.org/zeromq/zyre)|[![Build status](https://ci.appveyor.com/api/projects/status/kuugogjji97yblqe?svg=true)](https://ci.appveyor.com/project/zeromq/zyre)|
## Contents

.toc 3

## Overview

### Scope and Goals

Zyre provides reliable group messaging over local area networks. It has these key characteristics:

* Zyre needs no administration or configuration.
* Peers may join and leave the network at any time.
* Peers talk to each other without any central brokers or servers.
* Peers can talk directly to each other.
* Peers can join groups, and then talk to groups.
* Zyre is reliable, and loses no messages even when the network is heavily loaded.
* Zyre is fast and has low latency, requiring no consensus protocols.
* Zyre is designed for WiFi networks, yet also works well on Ethernet networks.
* Time for a new peer to join a network is about one second.

Typical use cases for Zyre are:

* Local service discovery.
* Clustering of a set of services on the same Ethernet network.
* Controlling a network of smart devices (Internet of Things).
* Multi-user mobile applications (like smart classrooms).

Technical details:

* Uses RFC 36 (http://rfc.zeromq.org/spec:36/ZRE) protocol for discovery and heartbeating.
* Uses reliable Dealer-Router pattern for interconnection, assuring that messages are not lost unless a peer application terminates.
* Optimized for WiFi, using UDP broadcasts for discovery and heartbeating…
* Offers alternative discovery mechanism (gossip) for Ethernet networks.

### Ownership and License

The contributors are listed in AUTHORS. This project uses the MPL v2 license, see LICENSE.

Zyre uses the [C4.1 (Collective Code Construction Contract)](http://rfc.zeromq.org/spec:22) process for contributions.

Zyre uses the [CLASS (C Language Style for Scalabilty)](http://rfc.zeromq.org/spec:21) guide for code style.

To report an issue, use the [Zyre issue tracker](https://github.com/zeromq/zyre/issues) at github.com.

## Using Zyre

### Building on Linux and macOS

To start with, you need at least these packages:
* `git` -- git is how we share code with other people.
* `build-essential`, `libtool`, `pkg-config` - the C compiler and related tools.
* `autotools-dev`, `autoconf`, `automake` - the GNU autoconf makefile generators.
* `cmake` - the CMake makefile generators (an alternative to autoconf).

Plus some others:
* `uuid-dev`, `libpcre3-dev` - utility libraries.
* `valgrind` - a useful tool for checking your code.
* `pkg-config` - an optional useful tool to make building with dependencies easier.

Which we install like this (using the Debian-style apt-get package manager):

```
    sudo apt-get update
    sudo apt-get install -y \
    git build-essential libtool \
    pkg-config autotools-dev autoconf automake cmake \
    uuid-dev libpcre3-dev valgrind

    # only execute this next line if interested in updating the man pages as
    # well (adds to build time):
    sudo apt-get install -y asciidoc
```
Here's how to build Zyre from GitHub (building from packages is very similar, you don't clone a repo but unpack a tarball), including the libsodium (for security) and libzmq (ZeroMQ core) libraries:

```
    git clone --depth 1 -b stable https://github.com/jedisct1/libsodium.git
    cd libsodium
    ./autogen.sh && ./configure && make check
    sudo make install
    cd ..

    git clone git://github.com/zeromq/libzmq.git
    cd libzmq
    ./autogen.sh
    # do not specify "--with-libsodium" if you prefer to use internal tweetnacl
    # security implementation (recommended for development)
    ./configure --with-libsodium
    make check
    sudo make install
    sudo ldconfig
    cd ..

    git clone git://github.com/zeromq/czmq.git
    cd czmq
    ./autogen.sh && ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..

    git clone git://github.com/zeromq/zyre.git
    cd zyre
    ./autogen.sh && ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..
```


Test by running the `zyre_selftest` command:

    zyre\src\.libs\zyre_selftest

Test by running the `zpinger` command, from two or more PCs.

    zyre\src\.libs\zpinger

### Building on Windows

To start with, you need MS Visual Studio (C/C++). The free community edition works well.

Then, install git, and make sure it works from a DevStudio command prompt:

```
git
```

Now let's build Zyre from GitHub:

```
    git clone --depth 1 -b stable https://github.com/jedisct1/libsodium.git
    git clone git://github.com/zeromq/libzmq.git
    git clone git://github.com/zeromq/czmq.git
    git clone git://github.com/zeromq/zyre.git
    cd zyre\builds\msvc
    configure.bat
    cd build
    buildall.bat
    cd ..\..\..\..
```

Test by running the `zyre_selftest` command:
```
    dir/s/b zyre_selftest.exe
    zyre\builds\msvc\vs2013\DebugDEXE\zyre_selftest.exe
    zyre\builds\msvc\vs2013\ReleaseDEXE\zyre_selftest.exe

    :: select your choice and run it
    zyre\builds\msvc\vs2013\DebugDEXE\zyre_selftest.exe
```
Test by running `zpinger` from two or more PCs:

```
    dir/s/b zpinger.exe
    zyre\builds\msvc\vs2013\DebugDEXE\zpinger.exe
    zyre\builds\msvc\vs2013\ReleaseDEXE\zpinger.exe
    zyre\builds\msvc\vs2013\x64\DebugDEXE\zpinger.exe

    :: select your choice and run it
    zyre\builds\msvc\vs2013\ReleaseDEXE\zpinger.exe
```

### Building on Windows

To start with, you need MS Visual Studio (C/C++). The free community edition works well.

Then, install git, and make sure it works from a DevStudio command prompt:

```
git
```

#### Using CMake

`zyre` requires `czmq` and `libzmq`, so we need to build `libzmq` first. For `libzmq`, you can optionally use [libsodium](https://github.com/jedisct1/libsodium) as the curve encryption library. So we will start from building `libsodium` in the following (and you can bypass the building of `libsodium` if you are ok with libzmq's default curve encryption library):

```
git clone --depth 1 -b stable https://github.com/jedisct1/libsodium.git
cd libsodium\builds\msvc\build
buildall.bat
cd ..\..\..\..
```

Once done, you can find the library files under `libsodium\bin\<Win32|x64>\<Debug|Release>\<Platform Toolset>\<dynamic|ltcg|static>`.

Here, the `<Platform Toolset>` is the platform toolset you are using: `v100` for `VS2010`, `v140` for `VS2015`, `v141` for `VS2017`, etc.

```
git clone git://github.com/zeromq/libzmq.git
cd libzmq
mkdir build
cd build
cmake .. -DBUILD_STATIC=OFF -DBUILD_SHARED=ON -DZMQ_BUILD_TESTS=ON -DWITH_LIBSODIUM=ON -DCMAKE_INCLUDE_PATH=..\libsodium\src\libsodium\include -DCMAKE_LIBRARY_PATH=..\libsodium\bin\Win32\Release\<Platform Toolset>\dynamic -DCMAKE_INSTALL_PREFIX=C:\projects\libs
cmake --build . --config Release --target install
cd ..\..\
```
`-DWITH_LIBSODIUM=ON` is necessary if you want to build `libzmq` with `libsodium`. `CMAKE_INCLUDE_PATH` option tells `libzmq` where to search for `libsodium`'s header files. And the `CMAKE_LIBRARY_PATH` option tells where to search for libsodium library files. If you don't need `libsodium` support, you can omit these three options.

`-DCMAKE_INSTALL_PREFIX=C:\libzmq` means we want to install `libzmq` into the `C:\libzmq`. You may need to run your shell with administrator privilege in order to write to the system disk.

Next, let's build `czmq`:

```
git clone git://github.com/zeromq/czmq.git
cd czmq
mkdir build
cd build
cmake .. -DCZMQ_BUILD_SHARED=ON -DCZMQ_BUILD_STATIC=OFF -DCMAKE_PREFIX_PATH=C:\projects\libs -DCMAKE_INSTALL_PREFIX=C:\projects\libs
cmake --build . --config Release --target install
```

Remember that we installed `libzmq` to `C:\projects\libs` through specifying `-DCMAKE_INSTALL_PREFIX` in the previous step. We here use `-DCMAKE_PREFIX_PATH=C:\projects\libs` to tell `czmq` where to search for `libzmq`.

That is not the whole story. We didn't mention the building of `libcurl`, `lz4`, `libuuid` and other `czmq` optional libraries above. In fact, to build all of these optional libraries successfully is really tricky. Please refer issue [#1972](https://github.com/zeromq/czmq/issues/1972) for more details.

Now, it is time to build `zyre`:

```
git clone git://github.com/zeromq/zyre.git
cd zyre
mkdir build
cd build
cmake .. -DZYRE_BUILD_SHARED=ON -DZYRE_BUILD_STATIC=OFF -DCMAKE_PREFIX_PATH=C:\projects\libs
cmake --build . --config Release
ctest -C Release
```

### Linking with an Application

Include `zyre.h` in your application and link with libzyre. Here is a typical gcc link command:

    gcc myapp.c -lzyre -lczmq -lzmq -o myapp

### Use from Other Languages

This is a list of auto-generated bindings:

* https://github.com/zeromq/zyre/tree/master/bindings/jni - Java ([Examples](https://github.com/zeromq/zyre/tree/master/examples/jni))
* https://github.com/zeromq/zyre/tree/master/bindings/nodejs - NodeJS
* https://github.com/zeromq/zyre/tree/master/bindings/python - Python
* https://github.com/zeromq/zyre/tree/master/bindings/python_cffi - Python (cffi)
* https://github.com/zeromq/zyre/tree/master/bindings/qml - QML
* https://github.com/zeromq/zyre/tree/master/bindings/qt - Qt
* https://github.com/zeromq/zyre/tree/master/bindings/ruby - Ruby (FFI)

### API Summary

This is the API provided by Zyre 2.x, in alphabetical order.

.pull doc/zyre.doc
.pull doc/zyre_event.doc

### Hints to Contributors

Zyre is a nice, neat library, and you may not immediately appreciate why. Read the CLASS style guide please, and write your code to make it indistinguishable from the rest of the code in the library. That is the only real criteria for good style: it's invisible.

Don't include system headers in source files. The right place for these is CZMQ.

Do read your code after you write it and ask, "Can I make this simpler?" We do use a nice minimalist and yet readable style. Learn it, adopt it, use it.

Before opening a pull request read our [contribution guidelines](https://github.com/zeromq/zyre/blob/master/CONTRIBUTING.md). Thanks!

### This Document
