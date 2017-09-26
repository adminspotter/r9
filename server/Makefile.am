SUBDIRS = classes

bin_PROGRAMS = r9d

r9d_SOURCES = server.cc signals.cc server.h signals.h
r9d_LDADD = classes/libr9_classes.la ../proto/libr9_proto.la $(SERVER_LDLIBS)

clean-local:
	rm -f *.gcno *.gcda *.gcov
