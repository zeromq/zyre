lib_LTLIBRARIES = libzre.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libzre.pc

libzre_la_includedir = $(includedir)
libzre_la_include_HEADERS = \
    ../include/zre.h \
    ../include/zre_msg.h \
    ../include/zre_peer.h \
    ../include/zre_group.h \
    ../include/zre_log.h \
    ../include/zre_log_msg.h \
    ../include/zre_uuid.h \
    ../include/zre_node.h

libzre_la_SOURCES = \
    zre_node.c \
    zre_group.c \
    zre_log.c \
    zre_log_msg.c \
    zre_msg.c \
    zre_peer.c \
    zre_uuid.c 

INCLUDES = -I$(top_srcdir)/include
bin_PROGRAMS = zyre_selftest zyre_ping zyre_tester zyre_logger zyre_perf_local zyre_perf_remote
zyre_selftest_LDADD = libzre.la
zyre_ping_LDADD = libzre.la
zyre_tester_LDADD = libzre.la
zyre_logger_LDADD = libzre.la
zyre_perf_local_LDADD = libzre.la
zyre_perf_remote_LDADD = libzre.la

if ON_MINGW
libzre_la_LDFLAGS = -no-undefined -avoid-version -version-info @LTVER@
else
libzre_la_LDFLAGS = -version-info @LTVER@
endif

TESTS = zyre_selftest
