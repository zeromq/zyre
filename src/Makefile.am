lib_LTLIBRARIES = libzyre.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libzyre.pc

libzyre_la_includedir = $(includedir)
libzyre_la_include_HEADERS = \
    ../include/zre_msg.h \
    ../include/zyre_event.h \
    ../include/zyre.h

libzyre_la_SOURCES = \
    zre_msg.c \
    zyre_peer.h \
    zyre_group.h \
    zyre_node.h \
    zyre_classes.h \
    zyre_group.c \
    zyre_peer.c \
    zyre_node.c \
    zyre_event.c \
    zyre.c

AM_CPPFLAGS = -I$(top_srcdir)/include
noinst_PROGRAMS = zyre_selftest 
zyre_selftest_LDADD = libzyre.la

if ON_MINGW
libzyre_la_LDFLAGS = -no-undefined -avoid-version -version-info @LTVER@
else
libzyre_la_LDFLAGS = -version-info @LTVER@
endif

TESTS = zyre_selftest

# Produce generated models; do this manually in src directory
code:
	gsl -q zre_msg.xml 
