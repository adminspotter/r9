if WANT_BUNDLED_LIBGLW
  LIBGLW_CFLAGS = -I@srcdir@/../../glw
  LIBGLW_LDADD = @srcdir@/../../glw/libGLw.a
else
  LIBGLW_CFLAGS =
  LIBGLW_LDADD =
endif

PIXMAPS_DIR = -DPIXMAP_PATH=\"@datadir@/$(PACKAGE)/pixmaps/\"

bin_PROGRAMS = r9client

r9client_SOURCES = about.cc command.cc client.cc menu.cc message.cc \
	msglog.cc setup.cc view.cc client.h msglog.h
r9client_CXXFLAGS = $(CLIENT_CXXFLAGS) $(PIXMAPS_DIR) $(LIBGLW_CFLAGS) \
	$(CLIENT_INCLUDES)
r9client_LDFLAGS = $(CLIENT_LDFLAGS)
r9client_LDADD = ../libr9_client.a $(LIBGLW_LDADD) \
	../../proto/libr9_proto.la $(CLIENT_LDLIBS)
