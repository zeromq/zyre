lib_LTLIBRARIES = libzre.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libzre.pc

libzre_la_includedir = $(includedir)/zre
libzre_la_include_HEADERS = \
    ../include/zre.h \
		../include/zre_udp.h \
		../include/zre_msg.h \
		../include/zre_peer.h \
		../include/zre_group.h \
		../include/zre_log.h \
		../include/zre_log_msg.h \
		../include/zre_interface.h


libzre_la_SOURCES = \
    zre_interface.c \
    zre_group.c \
    zre_log.c \
    zre_log_msg.c \
    zre_msg.c \
    zre_peer.c \
    zre_udp.c 

INCLUDES = -I$(top_srcdir)/include
bin_PROGRAMS = zre_ping zre_tester zre_logger zre_perf_local zre_perf_remote
zre_ping_LDADD = libzre.la
zre_tester_LDADD = libzre.la
zre_logger_LDADD = libzre.la
zre_perf_local_LDADD = libzre.la
zre_perf_remote_LDADD = libzre.la

if ON_MINGW
libzre_la_LDFLAGS = -no-undefined -avoid-version -version-info @LTVER@
else
libzre_la_LDFLAGS = -version-info @LTVER@
endif

TESTS = zyre_selftest
