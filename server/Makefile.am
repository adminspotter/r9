SUBDIRS = classes

bin_PROGRAMS = r9d
libexec_PROGRAMS = r9subserver

r9d_SOURCES = config_data.cc server.cc signals.cc log.cc config_data.h \
	server.h signals.h log.h
r9d_LDADD = classes/libr9_classes.la classes/modules/libr9_console.la \
	../proto/libr9_proto.la

r9subserver_SOURCES = subserver.cc log.cc log.h
