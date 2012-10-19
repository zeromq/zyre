lib_LTLIBRARIES = libzre.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libzre.pc

include_HEADERS = \
    ../include/zre.h

libzre_la_SOURCES = \
    zre_interface.c \
    zre_msg.c \
    zre_peer.c \
    zre_udp.c 

INCLUDES = -I$(top_srcdir)/include
bin_PROGRAMS = zre_ping
zre_ping_LDADD = libzre.la

if ON_MINGW
libzre_la_LDFLAGS = -no-undefined -avoid-version -version-info @LTVER@
else
libzre_la_LDFLAGS = -version-info @LTVER@
endif

TESTS = zyre_selftest
