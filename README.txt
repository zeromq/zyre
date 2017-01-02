.set GIT=https://github.com/zeromq/zyre
.sub 0MQ=ØMQ

[![GitHub release](https://img.shields.io/github/release/zeromq/zyre.svg)](https://github.com/zeromq/zyre/releases)
[![OBS draft](https://img.shields.io/badge/OBS%20master-draft-yellow.svg)](http://software.opensuse.org/download.html?project=home%3Azeromq%3Agit-draft&package=zyre)
[![OBS stable](https://img.shields.io/badge/OBS%20master-stable-yellow.svg)](http://software.opensuse.org/download.html?project=home%3Azeromq%3Agit-stable&package=zyre)
<a target="_blank" href="http://webchat.freenode.net?channels=%23zeromq&uio=d4"><img src="https://cloud.githubusercontent.com/assets/493242/14886493/5c660ea2-0d51-11e6-8249-502e6c71e9f2.png" height = "20" /></a>
[![license](https://img.shields.io/github/license/zeromq/zyre.svg)](https://github.com/zeromq/zyre/blob/master/LICENSE)

# Zyre - Local Area Clustering for Peer-to-Peer Applications

| Linux & MacOSX |
|:--------------:|
|[![Build Status](https://travis-ci.org/zeromq/zyre.png?branch=master)](https://travis-ci.org/zeromq/zyre)|

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

### Building on Linux

To start with, you need at least these packages:

* {{git-all}} -- git is how we share code with other people.

* {{build-essential}}, {{libtool}}, {{pkg-config}} - the C compiler and related tools.

* {{autotools-dev}}, {{autoconf}}, {{automake}} - the GNU autoconf makefile generators.

* {{cmake}} - the CMake makefile generators (an alternative to autoconf).

Plus some others:

* {{uuid-dev}}, {{libpcre3-dev}} - utility libraries.

* {{valgrind}} - a useful tool for checking your code.

Which we install like this (using the Debian-style apt-get package manager):

```
    sudo apt-get update
    sudo apt-get install -y \
    git-all build-essential libtool \
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

### Linking with an Application

Include `zyre.h` in your application and link with libzyre. Here is a typical gcc link command:

    gcc -lzyre -lczmq -lzmq myapp.c -o myapp

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
