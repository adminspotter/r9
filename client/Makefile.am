SUBDIRS = . motif pixmaps

noinst_LIBRARIES = libr9_client.a

dtddir = @datadir@/$(PACKAGE)/dtd

libr9_client_a_SOURCES = comm.cc client_core.cc \
	configdata.cc cache.h dtdresolver.h object.h comm.h \
	client_core.h configdata.h l10n.h
libr9_client_a_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES) \
	-DDTD_PATH=\"$(dtddir)/\" -DSTORE_PREFIX=\"@datadir@/$(PACKAGE)\" \
	-DGL_GLEXT_PROTOTYPES
