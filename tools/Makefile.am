EXTRA_DIST = zpinger.c \
             ztester_beacon.c \
             ztester_gossip.c \
             perf_local.c \
             perf_remote.c

AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = zpinger \
               ztester_beacon \
               ztester_gossip \
               perf_local \
               perf_remote

zpinger_LDADD = $(top_builddir)/src/libzyre.la
zpinger_SOURCES = zpinger.c

ztester_beacon_LDADD = $(top_builddir)/src/libzyre.la
ztester_beacon_SOURCES = ztester_beacon.c

ztester_gossip_LDADD = $(top_builddir)/src/libzyre.la
ztester_gossip_SOURCES = ztester_gossip.c

perf_local_LDADD = $(top_builddir)/src/libzyre.la
perf_local_SOURCES = perf_local.c

perf_remote_LDADD = $(top_builddir)/src/libzyre.la
perf_remote_SOURCES = perf_remote.c
