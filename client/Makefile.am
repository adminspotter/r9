SUBDIRS = . motif pixmaps

noinst_LIBRARIES = libr9_client.a

dtddir = @datadir@/$(PACKAGE)/dtd
dtd_DATA = geometry.dtd texture.dtd

libr9_client_a_SOURCES = geometry.cc object.cc texture.cc comm.cc \
	configdata.cc cache.h geometry.h object.h texture.h comm.h \
	configdata.h
libr9_client_a_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES)
