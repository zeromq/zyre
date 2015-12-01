cmake_minimum_required (VERSION 2.8)

project (zyrejni CXX)
enable_language (C)

find_package (JNI REQUIRED)
find_library (ZMQ_LIBRARY NAMES zmq libzmq REQUIRED)
find_library (ZYRE_LIBRARY NAMES zyre REQUIRED)

include_directories (${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2} src/native/include)

set (zyrejni_sources
    src/main/c/org_zeromq_zyre_Zyre.c
    src/main/c/org_zeromq_zyre_ZyreEvent.c
)

add_library (zyrejni SHARED ${zyrejni_sources})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -O2")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

target_link_libraries (zyrejni ${ZMQ_LIBRARY} ${ZYRE_LIBRARY})
