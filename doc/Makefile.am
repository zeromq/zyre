if BUILD_DOC

MAN1 = 
MAN3 = zyre.3 zyre_event.3 zyre_log.3 zre_msg.3 zre_log_msg.3
MAN7 = 
MAN_DOC = $(MAN1) $(MAN3) $(MAN7)

MAN_TXT = $(MAN1:%.1=%.txt)
MAN_TXT += $(MAN3:%.3=%.txt)
MAN_TXT += $(MAN7:%.7=%.txt)

if INSTALL_MAN
dist_man_MANS = $(MAN_DOC)
endif

EXTRA_DIST = asciidoc.conf mkman $(MAN_TXT)

MAINTAINERCLEANFILES = $(MAN_DOC)

dist-hook : $(MAN_DOC)

if BUILD_DOC
SUFFIXES=.txt .xml .1 .3 .7

.txt.xml:
	$(srcdir)/mkman $<
	asciidoc -d manpage -b docbook -f  $(srcdir)/asciidoc.conf \
		-azre_version=@PACKAGE_VERSION@ -o$@ $<
.xml.1:
	xmlto man $<
.xml.3:
	xmlto man $<
.xml.7:
	xmlto man $<
endif

endif
