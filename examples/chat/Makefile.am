EXTRA_DIST = chat.c

AM_CPPFLAGS = \
    ${libsodium_CFLAGS} \
    ${libzmq_CFLAGS} \
    ${czmq_CFLAGS} \
    -I$(srcdir)/include \
    -DZYRE_BUILD_DRAFT_API=1

bin_PROGRAMS = chat

chat_LDADD = $(top_builddir)/src/libzyre.la ${libsodium_LIBS} ${libzmq_LIBS} ${czmq_LIBS}
chat_SOURCES = chat.c
