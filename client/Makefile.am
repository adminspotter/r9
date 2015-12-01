SUBDIRS = . motif pixmaps geometry texture

noinst_LIBRARIES = libr9_client.a

dtddir = @datadir@/$(PACKAGE)/dtd
dist_dtd_DATA = geometry.dtd texture.dtd

libr9_client_a_SOURCES = geometry.cc texture.cc comm.cc client_core.cc \
	configdata.cc cache.h geometry.h object.h texture.h comm.h \
	client_core.h configdata.h l10n.h
libr9_client_a_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES) \
	-DDTD_PATH=\"$(dtddir)/\" -DSTORE_PREFIX=\"@datadir@/$(PACKAGE)/\"
