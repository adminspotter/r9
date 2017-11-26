SUBDIRS = ui shaders

bin_PROGRAMS = r9client

dtddir = @datadir@/$(PACKAGE)/dtd
shaderdir = @datadir@/$(PACKAGE)/shaders

r9client_SOURCES = client.cc client.h \
	comm.cc comm.h \
	client_core.cc client_core.h \
	shader.cc shader.h \
	configdata.cc configdata.h \
	log_display.cc log_display.h \
	comm_dialog.cc \
	cache.h dtdresolver.h object.h l10n.h
r9client_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES) $(LOCALE_DIR) \
	-DDTD_PATH=\"$(dtddir)/\" \
	-DSHADER_SRC_PATH=\"$(shaderdir)\" \
	-DSTORE_PREFIX=\"@datadir@/$(PACKAGE)\" \
	-DGL_GLEXT_PROTOTYPES
r9client_LDADD = ui/libr9_client_ui.la ../proto/libr9_proto.la $(CLIENT_LDLIBS)

clean-local:
	rm -f *.gcno *.gcda *.gcov
