#! /bin/bash
#
#   Builds Zyre dependencies
#
#   This is an experiment. The aim is to build all Zyre dependencies
#   into a single destination in bindings/nodejs. We want to get all the
#   libraries and header files into lib and include.
#
#   We expect projects to sit as siblings under a common root. So:
#
#       whatever/zyre
#       whatever/czmq
#       whatever/libzmq
#       whatever/libsodium
#
#   So these are both valid patterns:
#   - clone zyre, then in zyre/bindings/nodejs, run ./build.sh (classic CI)
#   - clone all projects, work, then build.sh in zyre/bindings/nodejs
#
#   The script tries hard to recover from interrupts, so that re-running
#   it takes the least time possible. In some cases this can get confused.
#   Then, rm -rf nodejs-build in each project.
#
#   After running this, you must re-configure in each project if you want
#   a normal build.

function build {
    PROJECT=$1
    REPO=$2
    BRANCH="${3:-master}"

    echo "I: building $PROJECT..."

    #   Get project from GitHub if it's not already here
    if [ ! -d $PROJECT ]; then
        if [ "$REPO" == "" ]; then
            echo "E:    $COMMON_ROOT/$PROJECT does not exist, aborting"
            exit
        else
            echo "I:    cloning $PROJECT into $COMMON_ROOT/$PROJECT..."
            git clone $QUIET $REPO
            ( cd $PROJECT; git checkout $BRANCH )
        fi
    fi

    cd $PROJECT
    if [ ! -f configure ]; then
        echo "I:     'autogen.sh' in `pwd`..."
        ./autogen.sh 2> /dev/null 1>&2
    fi

    #   We do an out of source build in nodejs-build
    if [ -f Makefile ]; then
        echo "I:     'make distclean' in `pwd`..."
        make -s distclean
    fi

    #   Create or reuse nodejs-build working directory
    test $FORCE -eq 1 && rm -rf nodejs-build
    mkdir -p nodejs-build
    cd nodejs-build

    if [ ! -f Makefile ]; then
        echo "I:     'configure' in `pwd`..."
        export CPPFLAGS="-I$BUILD_ROOT/deps/include"
        export LDFLAGS="-L$BUILD_ROOT/deps/lib"
        export PKG_CONFIG_PATH="$BUILD_ROOT/deps/lib/pkgconfig"
        CONFIG_OPTS="$QUIET --disable-shared --enable-static --with-pic --prefix=$BUILD_ROOT/deps"
        test -d ../doc && CONFIG_OPTS="$CONFIG_OPTS --without-docs"
        ../configure $CONFIG_OPTS
    fi
    CFLAGS="-fPIC"
    if [ $VERBOSE -eq 1 ]; then
        make -j 5 all
        make install
    else
        echo "I:     'make all' in `pwd`"
        make -j 5 all > build.log
        echo "I:     'make install' into $BUILD_ROOT/deps"
        make install >> build.log
    fi
}

#################################   Main   #################################

set -e                      #   exit on any error
export BUILD_ROOT=`pwd`
mkdir -p build
cd ../../..
COMMON_ROOT=`pwd`

FORCE=0
VERBOSE=0
ELECTRON=0
QUIET="--quiet"
LOGLEVEL="--loglevel=error"

for ARG in $*; do
    if [ "$ARG" == "--help" -o "$ARG" == "-h" ]; then
        echo "build.sh - rebuild zyre.node"
        echo " --help / -h          This help"
        echo " --force / -f         Force full rebuild"
        echo " --verbose / -v       Show build output"
        echo " --xverbose / -x      Extra verbose"
        echo " --electron / -e      Build for Electron"
        exit
    elif [ "$ARG" == "--force" -o "$ARG" == "-f" ]; then
        FORCE=1
    elif [ "$ARG" == "--electron" -o "$ARG" == "-e" ]; then
        ELECTRON=1
    elif [ "$ARG" == "--verbose" -o "$ARG" == "-v" ]; then
        VERBOSE=1
        QUIET=""
        LOGLEVEL=""
    elif [ "$ARG" == "--xverbose" -o "$ARG" == "-x" ]; then
        VERBOSE=1
        QUIET=""
        LOGLEVEL="--loglevel=verbose"
        set -x
    fi
done

echo "I: resolving dependencies for Zyre:"
( build libsodium https://github.com/jedisct1/libsodium.git stable )
( build libzmq https://github.com/zeromq/libzmq.git )
( build czmq https://github.com/zeromq/czmq.git )
( build zyre )

echo "I: building Node.js binding:"
cd $BUILD_ROOT
test ! -d node_modules/nan && npm install nan@latest --save
test ! -d node_modules/bindings && npm install bindings --save-dev

#   Still not sure of this
if [ $ELECTRON -eq 1 ]; then
    test ! -d node_modules/electron-rebuild && npm install electron-rebuild --save-dev
    test ! -d node_modules/electron-prebuilt && npm install electron-prebuilt --save-dev
fi

node-gyp $LOGLEVEL configure
node-gyp $LOGLEVEL build

#   Still not sure of this
if [ $ELECTRON -eq 1 ]; then
    ./node_modules/.bin/electron-rebuild
fi

