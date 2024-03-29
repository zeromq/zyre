#!/usr/bin/env bash
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
#
#   Exit if any step fails
set -e

# Use directory of current script as the working directory
cd "$( dirname "${BASH_SOURCE[0]}" )"
PROJECT_ROOT="$(cd ../.. && pwd)"
PROJECT_JNI_ROOT="${PROJECT_ROOT}/bindings/jni"

# Configuration
export NDK_VERSION="${NDK_VERSION:-android-ndk-r25}"
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-/tmp/${NDK_VERSION}}"
export MIN_SDK_VERSION=${MIN_SDK_VERSION:-21}
export ANDROID_BUILD_DIR="${ANDROID_BUILD_DIR:-${PWD}/.build}"
export ANDROID_DEPENDENCIES_DIR="${ANDROID_DEPENDENCIES_DIR:-${PWD}/.deps}"
export BUILD_PREFIX="${BUILD_PREFIX:-/tmp/jni_build}"

export TRAVIS_TAG="${TRAVIS_TAG:-no}"
export TRAVIS_OS_NAME="${TRAVIS_OS_NAME:-}"
export BINDING_OPTS="${BINDING_OPTS:-}"

export CI_CONFIG_QUIET="${CI_CONFIG_QUIET:-yes}"
export CI_TIME="${CI_TIME:-}"
export CI_TRACE="${CI_TRACE:-no}"

# By default, dependencies will be cloned to /tmp/tmp-deps.
# If you have your own source tree for XXX, uncomment its
# XXX_ROOT configuration line below, and provide its absolute tree:
#    export LIBZMQ_ROOT="<absolute_path_to_LIBZMQ_source_tree>"
#    export CZMQ_ROOT="<absolute_path_to_CZMQ_source_tree>"

########################################################################
# Preparation
########################################################################
# Get access to android_build functions and variables
# Perform some sanity checks and calculate some variables.
source "${PROJECT_ROOT}/builds/android/android_build_helper.sh"

# Initialize our dependency _ROOT variables:
android_init_dependency_root "libzmq"             # Check or initialize LIBZMQ_ROOT
android_init_dependency_root "czmq"               # Check or initialize CZMQ_ROOT

# Fetch required dependencies:
[ ! -d "${LIBZMQ_ROOT}" ]           && android_clone_library "LIBZMQ" "${LIBZMQ_ROOT}" "https://github.com/zeromq/libzmq.git" ""
[ ! -d "${CZMQ_ROOT}" ]             && android_clone_library "CZMQ" "${CZMQ_ROOT}" "https://github.com/zeromq/czmq.git" ""

android_download_ndk

case "$CI_TIME" in
    [Yy][Ee][Ss]|[Oo][Nn]|[Tt][Rr][Uu][Ee])
        CI_TIME="time -p " ;;
    [Nn][Oo]|[Oo][Ff][Ff]|[Ff][Aa][Ll][Ss][Ee])
        CI_TIME="" ;;
esac

case "$CI_TRACE" in
    [Nn][Oo]|[Oo][Ff][Ff]|[Ff][Aa][Ll][Ss][Ee])
        set +x ;;
    [Yy][Ee][Ss]|[Oo][Nn]|[Tt][Rr][Uu][Ee])
        set -x ;;
esac

CONFIG_OPTS=()
CONFIG_OPTS+=("CFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("CPPFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("CXXFLAGS=-I${BUILD_PREFIX}/include")
CONFIG_OPTS+=("LDFLAGS=-L${BUILD_PREFIX}/lib")
CONFIG_OPTS+=("PKG_CONFIG_PATH=${BUILD_PREFIX}/lib/pkgconfig")
CONFIG_OPTS+=("--prefix=${BUILD_PREFIX}")
CONFIG_OPTS+=("--with-docs=no")
[ "${CI_CONFIG_QUIET}" = "yes" ] && CONFIG_OPTS+=("--quiet")

GRADLEW_OPTS=()
GRADLEW_OPTS+=("-PbuildPrefix=$BUILD_PREFIX")
GRADLEW_OPTS+=("--info")

rm -rf /tmp/tmp-deps
mkdir -p /tmp/tmp-deps

########################################################################
# Clone and build native dependencies
########################################################################
[ -z "$CI_TIME" ] || echo "`date`: Starting build of dependencies (if any)..."

######################
#  Build native 'libzmq.so'
(
    android_build_library "LIBZMQ" "${LIBZMQ_ROOT}"
)


######################
#  Build native 'libczmq.so'
(
    android_build_library "CZMQ" "${CZMQ_ROOT}"
)

# Build jni dependency
( cd ${CZMQ_ROOT}/bindings/jni && TERM=dumb $CI_TIME ./gradlew publishToMavenLocal ${GRADLEW_OPTS[@]} ${CZMQ_GRADLEW_OPTS} )

######################
# Build native 'libzyre.so'
cd "${PROJECT_ROOT}"
[ -z "$CI_TIME" ] || echo "`date`: Starting build of currently tested project..."

(
    android_build_library "LIBZYRE" "${PROJECT_ROOT}"
)

[ -z "$CI_TIME" ] || echo "`date`: Build completed without fatal errors!"

########################################################################
#  Build and check the jni android binding
########################################################################
cd "${PROJECT_JNI_ROOT}"
[ "${TRAVIS_TAG}" = "yes" ] && IS_RELEASE="-PisRelease"

TERM=dumb $CI_TIME ./gradlew build jar ${GRADLEW_OPTS[@]} ${ZYRE_GRADLEW_OPTS} $IS_RELEASE
TERM=dumb $CI_TIME ./gradlew clean

if [ "$TRAVIS_OS_NAME" == "linux" ] && [ "$BINDING_OPTS" == "android" ]; then
    pushd zyre-jni/android
        $CI_TIME ./build.sh "arm"
        $CI_TIME ./build.sh "arm64"
        $CI_TIME ./build.sh "x86"
        $CI_TIME ./build.sh "x86_64"
    popd
fi

################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
