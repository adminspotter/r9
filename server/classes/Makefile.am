SUBDIRS = actions modules

lib_LTLIBRARIES = libr9_classes.la

libr9_classes_la_SOURCES = control.cc control.h \
	game_obj.cc game_obj.h \
	geometry.cc geometry.h \
	zone.cc zone.h \
	action_pool.cc action_pool.h \
	motion_pool.cc motion_pool.h \
	update_pool.cc update_pool.h \
	library.cc library.h \
	basesock.cc basesock.h listensock.cc listensock.h \
	dgram.cc dgram.h stream.cc stream.h socket.cc socket.h \
	octree.cc octree.h \
	defs.h thread_pool.h
libr9_classes_la_CXXFLAGS = -DSUBSERVER_PROG=\"@libexecdir@/r9subserver\"
libr9_classes_la_LIBADD = ../../proto/libr9_proto.la
