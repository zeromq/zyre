#!/usr/bin/env bash

if [ $BUILD_TYPE == "default" ]; then
    #   libsodium
    git clone git://github.com/jedisct1/libsodium.git
    ( cd libsodium && ./autogen.sh && ./configure && make check && sudo make install && sudo ldconfig )

    #   libzmq
    git clone git://github.com/zeromq/libzmq.git
    ( cd libzmq && ./autogen.sh && ./configure && make check && sudo make install && sudo ldconfig )

    #   CZMQ
    git clone git://github.com/zeromq/czmq.git
    ( cd czmq && ./autogen.sh && ./configure && make check && sudo make install && sudo ldconfig )

    #   Build and check this project
    ./autogen.sh && ./configure && make && make check && sudo make install
else
    cd ./builds/${BUILD_TYPE} && ./ci_build.sh
fi
