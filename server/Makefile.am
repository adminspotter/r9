SUBDIRS = classes

bin_PROGRAMS = r9d
libexec_PROGRAMS = r9subserver

r9d_SOURCES = config_data.cc server.cc signals.cc log.cc config_data.h \
	server.h signals.h log.h
r9d_CXXFLAGS = -DSERVER_ROOT_DIR=\"$(prefix)\" \
	-DSERVER_PID_FNAME=\"$(localstatedir)/run/r9.pid\"
r9d_LDADD = classes/libr9_classes.la ../proto/libr9_proto.la $(SERVER_LDLIBS)

r9subserver_SOURCES = subserver.cc log.cc log.h
r9subserver_CXXFLAGS = -DIN_SUBSERVER
