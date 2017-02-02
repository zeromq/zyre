#!/usr/bin/env bash

set -x

# Go back to the root and build, just like the default target Only difference
# is meant to be the final step, where we make install on this project, then
# run the python tests.
cd ../..
mkdir tmp
BUILD_PREFIX=$PWD/tmp

CONFIG_OPTS=()
CONFIG_OPTS+=("CFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("CPPFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("CXXFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("LDFLAGS=-L${BUILD_PREFIX}/lib")
CONFIG_OPTS+=("PKG_CONFIG_PATH=${BUILD_PREFIX}/lib/pkgconfig")
CONFIG_OPTS+=("--prefix=${BUILD_PREFIX}")
CONFIG_OPTS+=("--with-docs=no")
CONFIG_OPTS+=("--quiet")

# Clone and build dependencies
git clone --quiet --depth 1 https://github.com/zeromq/libzmq libzmq
cd libzmq
git --no-pager log --oneline -n1
if [ -e autogen.sh ]; then
    ./autogen.sh 2> /dev/null
fi
if [ -e buildconf ]; then
    ./buildconf 2> /dev/null
fi
./configure "${CONFIG_OPTS[@]}"
make -j4
make install
cd ..
git clone --quiet --depth 1 https://github.com/zeromq/czmq czmq
cd czmq
git --no-pager log --oneline -n1
if [ -e autogen.sh ]; then
    ./autogen.sh 2> /dev/null
fi
if [ -e buildconf ]; then
    ./buildconf 2> /dev/null
fi
./configure "${CONFIG_OPTS[@]}"
make -j4
make install
cd ..

# Build and install this project
./autogen.sh 2> /dev/null
./configure --enable-drafts=yes "${CONFIG_OPTS[@]}"
make VERBOSE=1 install

# Copy dependency python bindings
cp -r czmq/bindings/python/czmq bindings/python/

# Setup environment & run tests
LD_LIBRARY_PATH=$PWD/tmp/lib python bindings/python/test.py
LD_LIBRARY_PATH=$PWD/tmp/lib python3 bindings/python/test.py
