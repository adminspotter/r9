GMOCK_DIR = gmock-1.7.0

SUBDIRS = $(GMOCK_DIR)

GTEST_LIBS = $(GMOCK_DIR)/gtest/lib/libgtest.la
GMOCK_LIBS = $(GMOCK_DIR)/lib/libgmock.la $(GTEST_LIBS)

TC = t_basesock t_console t_library t_log t_sockaddr t_subserver t_threadpool
if WANT_PERL
  TC += t_perl
endif
if WANT_TCL
  TC += t_tcl
endif
if WANT_CLIENT
  TC += t_cache t_configdata t_texture_parser
endif

TESTS = $(TC)

check_PROGRAMS = $(TC)
lib_LTLIBRARIES = libtest.la
EXTRA_DIST = t_texture_parser.xml

install-exec-local:
	@echo "Nothing to install"
	exit

AM_CXXFLAGS = -I$(GMOCK_DIR)/gtest/include -I$(GMOCK_DIR)/include

libtest_la_SOURCES = libtest.cc libtest.h
libtest_la_LDFLAGS = -module -rpath /nowhere

t_basesock_SOURCES = t_basesock.cc tap.h tap_main.cc \
	../server/classes/basesock.cc ../server/classes/basesock.h
t_basesock_LDADD = $(GTEST_LIBS)

CONFIG_DEFS =
CONSOLE_EXTRAS =
if HAVE_LIBWRAP
  CONFIG_DEFS += -DSERVER_ROOT_DIR=\"./\" -DSERVER_PID_FNAME=\"blah.pid\" \
	-DHAVE_LIBWRAP=1
  CONSOLE_EXTRAS += ../server/config_data.cc ../server/config_data.h
endif

t_console_SOURCES = t_console.cc tap.h tap_main.cc \
	../server/classes/modules/console.h \
	../server/classes/modules/console.cc \
	../server/classes/basesock.cc ../server/classes/basesock.h \
	$(CONSOLE_EXTRAS)
t_console_CXXFLAGS = $(CONFIG_DEFS) \
	-I$(GMOCK_DIR)/gtest/include -I$(GMOCK_DIR)/include
t_console_LDADD = $(GTEST_LIBS) $(WRAP_LDLIBS)

t_library_SOURCES = t_library.cc tap.h tap_main.cc \
	../server/classes/library.cc ../server/classes/library.h
t_library_LDADD = $(GTEST_LIBS)

t_log_SOURCES = t_log.cc tap.h \
	../server/log.cc ../server/log.h
t_log_LDADD = $(GTEST_LIBS)

t_sockaddr_SOURCES = t_sockaddr.cc tap.h tap_main.cc \
	../server/classes/sockaddr.h
t_sockaddr_LDADD = $(GTEST_LIBS)

t_subserver_SOURCES = t_subserver.cc tap.h tap_main.cc ../server/r9subserver
t_subserver_LDADD = $(GTEST_LIBS)

t_threadpool_SOURCES = t_threadpool.cc tap.h tap_main.cc \
	../server/classes/thread_pool.h
t_threadpool_LDADD = $(GTEST_LIBS)

t_perl_SOURCES = t_perl.cc tap.h ../server/classes/modules/language.h \
	../server/classes/library.h ../server/classes/library.cc
t_perl_LDADD = $(GTEST_LIBS)

t_tcl_SOURCES = t_tcl.cc tap.h ../server/classes/modules/language.h \
	../server/classes/library.h ../server/classes/library.cc
t_tcl_LDADD = $(GTEST_LIBS)

t_cache_SOURCES = t_cache.cc tap.h tap_main.cc ../client/cache.h
t_cache_LDADD = $(GTEST_LIBS) $(GMOCK_LIBS) $(CLIENT_LDLIBS)

t_configdata_SOURCES = t_configdata.cc tap.h tap_main.cc \
	../client/configdata.h ../client/configdata.cc
t_configdata_LDADD = $(GTEST_LIBS) $(CLIENT_LDLIBS)

t_texture_parser_SOURCES = t_texture_parser.cc tap.h tap_main.cc \
	../client/texture.cc ../client/texture.h
t_texture_parser_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES) \
	-DDTD_PATH=\"../client\" \
	-I$(GMOCK_DIR)/gtest/include -I$(GMOCK_DIR)/include
t_texture_parser_LDADD = $(GTEST_LIBS) $(CLIENT_LDLIBS)
