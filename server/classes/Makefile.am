SUBDIRS = actions modules

serverlibdir = $(pkglibdir)/server
actionlibdir = $(serverlibdir)/actions

lib_LTLIBRARIES = libr9_classes.la

libr9_classes_la_SOURCES = action.h action_pool.cc action_pool.h \
	basesock.cc basesock.h \
	config_data.cc config_data.h \
	control.cc control.h \
	dgram.cc dgram.h \
	game_obj.cc game_obj.h \
	geometry.cc geometry.h \
	library.cc library.h \
	listensock.cc listensock.h \
	log.cc log.h \
	motion_pool.cc motion_pool.h \
	octree.cc octree.h \
	socket.cc socket.h \
	stream.cc stream.h \
	update_pool.cc update_pool.h \
	zone.cc zone.h \
	thread_pool.h
libr9_classes_la_CXXFLAGS = -DSERVER_ROOT_DIR=\"$(prefix)\" \
	-DSERVER_LIB_DIR=\"$(serverlibdir)\" \
	-DACTION_LIB_DIR=\"$(actionlibdir)\" \
	-DSERVER_PID_FNAME=\"$(localstatedir)/run/r9.pid\"
libr9_classes_la_LIBADD = ../../proto/libr9_proto.la

clean-local:
	rm -f *.gcno *.gcda *.gcov
