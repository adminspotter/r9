SUBDIRS = shaders
if WANT_CUDDLY
  SUBDIRS += cuddly-gl
endif

bin_PROGRAMS = r9client

shaderdir = @datadir@/$(PACKAGE)/shaders

r9client_SOURCES = client.cc client.h \
	comm.cc comm.h \
	client_core.cc client_core.h \
	shader.cc shader.h \
	configdata.cc configdata.h \
	log_display.cc log_display.h \
	comm_dialog.cc \
	object.h l10n.h cuddly-gl/connect_glfw.cc
r9client_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES) $(LOCALE_DIR) \
	$(X11_DEFS) \
	-DSHADER_SRC_PATH=\"$(shaderdir)\" \
	-DGL_GLEXT_PROTOTYPES
r9client_LDFLAGS= $(CLIENT_LDFLAGS)
r9client_LDADD = $(CLIENT_LDLIBS) ../proto/libr9_proto.la
if WANT_CUDDLY
  r9client_CXXFLAGS += -I.
  r9client_LDADD += cuddly-gl/libcuddly_gl.la
endif

check: SUBDIRS =

clean-local:
	rm -f *.gcno *.gcda *.gcov
