SUBDIRS = classes

bin_PROGRAMS = r9d

r9d_SOURCES = config_data.cc server.cc signals.cc log.cc config_data.h \
	server.h signals.h log.h
r9d_CXXFLAGS = -DSERVER_ROOT_DIR=\"$(prefix)\" \
	-DSERVER_PID_FNAME=\"$(localstatedir)/run/r9.pid\"
r9d_LDADD = classes/libr9_classes.la ../proto/libr9_proto.la $(SERVER_LDLIBS)

clean-local:
	rm -f *.gcno *.gcda *.gcov
