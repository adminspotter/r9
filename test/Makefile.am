GMOCK_DIR = gmock-1.7.0

SUBDIRS = $(GMOCK_DIR)

GTEST_INCLUDES = -I$(GMOCK_DIR)/gtest/include
GMOCK_INCLUDES = -I$(GMOCK_DIR)/include
GTEST_LIBS = $(GMOCK_DIR)/gtest/lib/libgtest.la
GMOCK_LIBS = $(GMOCK_DIR)/lib/libgmock.la $(GTEST_LIBS)
GTEST_MAIN_OBJ = $(GMOCK_DIR)/gtest/src/gtest_main.o
GMOCK_MAIN_OBJ = $(GMOCK_DIR)/src/gmock_main.o

TC = t_action_pool \
	t_basesock \
	t_console \
	t_library \
	t_log \
	t_motion_pool \
	t_sockaddr \
	t_subserver \
	t_threadpool \
	t_update_pool
if WANT_PERL
  TC += t_perl
endif
if WANT_TCL
  TC += t_tcl
endif
if WANT_CLIENT
  TC += t_cache t_comm t_configdata
endif

TESTS = $(TC)

check_PROGRAMS = $(TC)
lib_LTLIBRARIES = libtest.la
EXTRA_DIST = t_texture_parser.xml

check: SUBDIRS =

install-exec-local:
	@echo "Nothing to install"
	false

AM_CXXFLAGS = $(GMOCK_INCLUDES) $(GTEST_INCLUDES)

CONFIG_DEFS = -DSERVER_ROOT_DIR=\"./\" -DSERVER_PID_FNAME=\"blah.pid\"
CONSOLE_EXTRAS =
if HAVE_LIBWRAP
  CONFIG_DEFS += -DHAVE_LIBWRAP=1
  CONSOLE_EXTRAS += ../server/config_data.cc ../server/config_data.h
endif

libtest_la_SOURCES = libtest.cc libtest.h
libtest_la_LDFLAGS = -module -rpath /nowhere

t_action_pool_SOURCES = t_action_pool.cc \
	../server/classes/action_pool.cc ../server/classes/action_pool.h \
	../server/classes/motion_pool.cc ../server/classes/motion_pool.h \
	../server/classes/update_pool.cc ../server/classes/update_pool.h \
	../server/classes/zone.cc ../server/classes/zone.h \
	../server/classes/control.cc ../server/classes/control.h \
	../server/classes/game_obj.cc ../server/classes/game_obj.h \
	../server/classes/library.cc ../server/classes/library.h \
	../server/classes/octree.cc ../server/classes/octree.h \
	../server/log.cc ../server/log.h \
	../server/config_data.cc ../server/config_data.h
t_action_pool_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_action_pool_LDADD = $(GMOCK_MAIN_OBJ) $(GMOCK_LIBS) $(GTEST_LIBS) \
	../proto/libr9_proto.la

t_basesock_SOURCES = t_basesock.cc \
	../server/classes/basesock.cc ../server/classes/basesock.h
t_basesock_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_console_SOURCES = t_console.cc \
	../server/classes/modules/console.h \
	../server/classes/modules/console.cc \
	../server/classes/basesock.cc ../server/classes/basesock.h \
	$(CONSOLE_EXTRAS)
t_console_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_console_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) $(WRAP_LDLIBS)

t_library_SOURCES = t_library.cc \
	../server/classes/library.cc ../server/classes/library.h
t_library_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_log_SOURCES = t_log.cc ../server/log.cc ../server/log.h \
	../server/config_data.cc ../server/config_data.h
t_log_CXXFLAGS = $(CONFIG_DEFS) $(GTEST_INCLUDES)
t_log_LDADD = $(GTEST_LIBS)

t_motion_pool_SOURCES = t_motion_pool.cc \
	../server/classes/action_pool.cc ../server/classes/action_pool.h \
	../server/classes/motion_pool.cc ../server/classes/motion_pool.h \
	../server/classes/update_pool.cc ../server/classes/update_pool.h \
	../server/classes/zone.cc ../server/classes/zone.h \
	../server/classes/game_obj.cc ../server/classes/game_obj.h \
	../server/classes/library.cc ../server/classes/library.h \
	../server/classes/octree.cc ../server/classes/octree.h \
	../server/log.cc ../server/log.h \
	../server/config_data.cc ../server/config_data.h
t_motion_pool_CXXFLAGS = $(CONFIG_DEFS) $(GTEST_INCLUDES)
t_motion_pool_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) ../proto/libr9_proto.la

t_sockaddr_SOURCES = t_sockaddr.cc ../server/classes/sockaddr.h
t_sockaddr_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_subserver_SOURCES = t_subserver.cc ../server/r9subserver
t_subserver_CXXFLAGS = -DIN_SUBSERVER $(GTEST_INCLUDES)
t_subserver_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_threadpool_SOURCES = t_threadpool.cc ../server/classes/thread_pool.h
t_threadpool_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_update_pool_SOURCES = t_update_pool.cc \
	../server/classes/action_pool.cc ../server/classes/action_pool.h \
	../server/classes/motion_pool.cc ../server/classes/motion_pool.h \
	../server/classes/update_pool.cc ../server/classes/update_pool.h \
	../server/classes/zone.cc ../server/classes/zone.h \
	../server/classes/listensock.cc ../server/classes/listensock.h \
	../server/classes/basesock.cc ../server/classes/basesock.h \
	../server/classes/control.cc ../server/classes/control.h \
	../server/classes/game_obj.cc ../server/classes/game_obj.h \
	../server/classes/library.cc ../server/classes/library.h \
	../server/classes/octree.cc ../server/classes/octree.h \
	../server/log.cc ../server/log.h \
	../server/config_data.cc ../server/config_data.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_update_pool_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_update_pool_LDADD = $(GMOCK_MAIN_OBJ) $(GTEST_LIBS) $(GMOCK_LIBS) \
	../proto/libr9_proto.la

t_perl_SOURCES = t_perl.cc ../server/classes/modules/language.h \
	../server/classes/library.h ../server/classes/library.cc
t_perl_LDADD = $(GTEST_LIBS)

t_tcl_SOURCES = t_tcl.cc ../server/classes/modules/language.h \
	../server/classes/library.h ../server/classes/library.cc
t_tcl_LDADD = $(GTEST_LIBS)

t_cache_SOURCES = t_cache.cc ../client/cache.h
t_cache_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES) \
	$(GTEST_INCLUDES) $(GMOCK_INCLUDES) \
	-DSTORE_PREFIX=\"../client\"
t_cache_LDADD = $(GMOCK_MAIN_OBJ) $(GTEST_LIBS) $(GMOCK_LIBS) $(CLIENT_LDLIBS)

t_comm_SOURCES = t_comm.cc ../client/comm.cc ../client/comm.h
t_comm_LDADD = $(GMOCK_MAIN_OBJ) $(GMOCK_LIBS) $(GTEST_LIBS) $(CLIENT_LDLIBS) \
	../proto/libr9_proto.la

t_configdata_SOURCES = t_configdata.cc \
	../client/configdata.h ../client/configdata.cc
t_configdata_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) $(CLIENT_LDLIBS)

t_texture_parser_SOURCES = t_texture_parser.cc \
	../client/texture.cc ../client/texture.h
t_texture_parser_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES) \
	-DDTD_PATH=\"../client/\" -DSTORE_PREFIX=\"../client\" \
	$(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_texture_parser_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) $(CLIENT_LDLIBS)
