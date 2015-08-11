SUBDIRS = motif

if WANT_BUNDLED_XERCES
  XERCES_CXXFLAGS = -I@srcdir@/../xerces-c/src
else
  XERCES_CXXFLAGS =
endif

noinst_LIBRARIES = libr9_client.a

libr9_client_a_SOURCES = geometry.cc texture.cc comm.cc configdata.cc \
	cache.h geometry.h texture.h comm.h configdata.h
libr9_client_a_CXXFLAGS = $(XERCES_CXXFLAGS) $(CLIENT_CXXFLAGS)
