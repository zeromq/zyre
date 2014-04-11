#################################################################
#   GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY    #
#   Please read the README.txt file in the model directory.     #
#################################################################

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8)
project(zyre)
enable_language(C)
enable_testing()

set(ZYRE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ZYRE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

########################################################################
# determine version
########################################################################
foreach(which MAJOR MINOR PATCH)
    file(STRINGS "${ZYRE_SOURCE_DIR}/include/zyre.h" ZYRE_VERSION_STRING REGEX "#define ZYRE_VERSION_${which}")
    string(REGEX MATCH "#define ZYRE_VERSION_${which} ([0-9_]+)" ZYRE_REGEX_MATCH "${ZYRE_VERSION_STRING}")
    if (NOT ZYRE_REGEX_MATCH)
        message(FATAL_ERROR "failed to parse ZYRE_VERSION_${which} from zyre.h")
    endif()
    set(ZYRE_${which}_VERSION ${CMAKE_MATCH_1})
endforeach(which)

set(ZYRE_VERSION ${ZYRE_MAJOR_VERSION}.${ZYRE_MINOR_VERSION}.${ZYRE_PATCH_VERSION})

########################################################################
# platform.h
########################################################################
include(CheckIncludeFile)
CHECK_INCLUDE_FILE("linux/wireless.h" HAVE_LINUX_WIRELESS_H)
CHECK_INCLUDE_FILE("net/if.h" HAVE_NET_IF_H)
CHECK_INCLUDE_FILE("net/if_media.h" HAVE_NET_IF_MEDIA_H)

include(CheckFunctionExists)
CHECK_FUNCTION_EXISTS("getifaddrs" HAVE_GETIFADDRS)
CHECK_FUNCTION_EXISTS("freeifaddrs" HAVE_FREEIFADDRS)

file(WRITE ${ZYRE_BINARY_DIR}/platform.h.in "
#cmakedefine HAVE_LINUX_WIRELESS_H
#cmakedefine HAVE_NET_IF_H
#cmakedefine HAVE_NET_IF_MEDIA_H
#cmakedefine HAVE_GETIFADDRS
#cmakedefine HAVE_FREEIFADDRS
")

configure_file(${ZYRE_BINARY_DIR}/platform.h.in ${ZYRE_BINARY_DIR}/platform.h)

#The MSVC C compiler is too out of date,
#so the sources have to be compiled as c++
if (MSVC)
    enable_language(CXX)
    file(GLOB sources ${ZYRE_SOURCE_DIR}/src/*.c)
    set_source_files_properties(${sources} PROPERTIES LANGUAGE CXX)
    set(MORE_LIBRARIES ws2_32 Rpcrt4 Iphlpapi)
endif()

########################################################################
# zeromq depedency
########################################################################
list(APPEND CMAKE_MODULE_PATH ${ZYRE_SOURCE_DIR})
find_package(ZeroMQ REQUIRED)
find_package(CZMQ REQUIRED)

########################################################################
# includes
########################################################################
set (zyre_headers
    include/zyre.h
    include/zyre_event.h
    include/zyre_log.h
    include/zre_log_msg.h
    include/zre_msg.h
)
source_group ("Header Files" FILES ${zyre_headers})
install(FILES ${zyre_headers} DESTINATION include)

########################################################################
# library
########################################################################
include_directories(${ZYRE_BINARY_DIR})
include_directories(${ZYRE_SOURCE_DIR}/include)
include_directories(${CZMQ_INCLUDE_DIRS})
include_directories(${ZEROMQ_INCLUDE_DIRS})
set (zyre_sources
    src/zre_log_msg.c
    src/zre_msg.c
    src/zyre.c
    src/zyre_event.c
    src/zyre_group.c
    src/zyre_log.c
    src/zyre_node.c
    src/zyre_peer.c
    src/zyre_selftest.c
)
source_group ("Source Files" FILES ${zyre_sources})
add_library(zyre SHARED ${zyre_sources})
set_target_properties(zyre PROPERTIES DEFINE_SYMBOL "LIBCZMQ_EXPORTS")
target_link_libraries(zyre ${CZMQ_LIBRARIES} ${ZEROMQ_LIBRARIES} ${MORE_LIBRARIES})

install(TARGETS zyre
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# pkgconfig
########################################################################
set(VERSION "${ZYRE_VERSION}")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
set(libdir "\${prefix}/lib${LIB_SUFFIX}")
set(includedir "\${prefix}/include")
configure_file(
    ${ZYRE_SOURCE_DIR}/src/libzyre.pc.in
    ${ZYRE_BINARY_DIR}/libzyre.pc
@ONLY)

install(
    FILES ${ZYRE_BINARY_DIR}/libzyre.pc
    DESTINATION lib${LIB_SUFFIX}/pkgconfig
)

########################################################################
# tests
########################################################################
add_executable(zyre_selftest ${ZYRE_SOURCE_DIR}/src/zyre_selftest.c)
target_link_libraries(zyre_selftest zyre ${CZMQ_LIBRARIES} ${ZEROMQ_LIBRARIES})
add_test(zyre_selftest zyre_selftest)

########################################################################
# summary
########################################################################
message(STATUS "version: ${ZYRE_VERSION}")
message(STATUS "install: ${CMAKE_INSTALL_PREFIX}")

#################################################################
#   GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY    #
#   Please read the README.txt file in the model directory.     #
#################################################################
