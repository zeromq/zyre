EXTRA_DIST = pinger.c \
             tester.c \
             logger.c \
             perf_local.c \
             perf_remote.c

INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = pinger \
               tester \
               logger \
               perf_local \
               perf_remote

pinger_LDADD = $(top_srcdir)/src/libzyre.la
pinger_SOURCES = pinger.c

tester_LDADD = $(top_srcdir)/src/libzyre.la
tester_SOURCES = tester.c

logger_LDADD = $(top_srcdir)/src/libzyre.la
logger_SOURCES = logger.c

perf_local_LDADD = $(top_srcdir)/src/libzyre.la
perf_local_SOURCES = perf_local.c

perf_remote_LDADD = $(top_srcdir)/src/libzyre.la
perf_remote_SOURCES = perf_remote.c
