#!/usr/bin/env bash

set -x

mkdir tmp
BUILD_PREFIX=$PWD/tmp

CONFIG_OPTS=()
CONFIG_OPTS+=("CFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("CPPFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("CXXFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("LDFLAGS=-L${BUILD_PREFIX}/lib")
CONFIG_OPTS+=("PKG_CONFIG_PATH=${BUILD_PREFIX}/lib/pkgconfig")
CONFIG_OPTS+=("--prefix=${BUILD_PREFIX}")

CMAKE_OPTS=()
CMAKE_OPTS+=("-DCMAKE_INSTALL_PREFIX:PATH=${BUILD_PREFIX}")
CMAKE_OPTS+=("-DCMAKE_PREFIX_PATH:PATH=${BUILD_PREFIX}")
CMAKE_OPTS+=("-DCMAKE_LIBRARY_PATH:PATH=${BUILD_PREFIX}/lib")
CMAKE_OPTS+=("-DCMAKE_INCLUDE_PATH:PATH=${BUILD_PREFIX}/include")

# Clone and build dependencies
git clone --depth 1 https://github.com/jedisct1/libsodium libsodium
( cd libsodium && ./autogen.sh && ./configure "${CONFIG_OPTS[@]}" && make -j4 && make install ) || exit 1

git clone --depth 1 https://github.com/zeromq/libzmq libzmq
( cd libzmq && ./autogen.sh && ./configure "${CONFIG_OPTS[@]}" && make -j4 && make install ) || exit 1

git clone --depth 1 https://github.com/zeromq/czmq czmq
( cd czmq && ./autogen.sh && ./configure "${CONFIG_OPTS[@]}" && make -j4 && make install ) || exit 1

# Build and check this project
( cd ../..; mkdir build_cmake && cd build_cmake && PKG_CONFIG_PATH=${BUILD_PREFIX}/lib/pkgconfig cmake "${CMAKE_OPTS[@]}" .. && make all VERBOSE=1 && make install ) || exit 1

