bin_PROGRAMS = r9d
libexec_PROGRAMS = r9subserver

r9d_SOURCES = config_data.cc server.cc signals.cc log.cc config_data.h \
	server.h signals.h subserver.h log.h
r9d_LDADD = classes/libr9_classes.la classes/modules/libr9_console.la \
	../proto/libr9_proto.la

r9subserver_SOURCES = subserver.cc log.cc subserver.h log.h

../proto/libr9_proto.la :
	$(MAKE) -C ../proto libr9_proto.la

classes/libr9_classes.la :
	$(MAKE) -C classes libr9_classes.la

classes/modules/libr9_console.la :
	$(MAKE) -C classes/modules libr9_console.la

install-exec-hook :
	$(MAKE) -C classes install
