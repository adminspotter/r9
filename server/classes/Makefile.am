SUBDIRS = actions modules

lib_LTLIBRARIES = libr9_classes.la

libr9_classes_la_SOURCES = control.cc game_obj.cc geometry.cc zone.cc \
	library.cc motion.cc basesock.cc listensock.cc dgram.cc stream.cc \
	octree.cc control.h defs.h game_obj.h geometry.h motion.h library.h \
	thread_pool.h zone.h basesock.h dgram.h stream.h octree.h
libr9_classes_la_CXXFLAGS = -DSUBSERVER_PROG=\"@libexecdir@/r9subserver\"
libr9_classes_la_LIBADD = ../../proto/libr9_proto.la
