GMOCK_DIR = gmock-1.7.0

SUBDIRS = tap++ $(GMOCK_DIR) .

GTEST_INCLUDES = -I$(GMOCK_DIR)/gtest/include
GMOCK_INCLUDES = -I$(GMOCK_DIR)/include
GTEST_LIBS = $(GMOCK_DIR)/gtest/lib/libgtest.la
GMOCK_LIBS = $(GMOCK_DIR)/lib/libgmock.la $(GTEST_LIBS)
GTEST_MAIN_OBJ = $(GMOCK_DIR)/gtest/src/gtest_main.o
GMOCK_MAIN_OBJ = $(GMOCK_DIR)/src/gmock_main.o

TAP_INCLUDES = -Itap++
TAP_LDADD = tap++/libtap++.la

TC =
if WANT_SERVER
  TC += t_action_pool \
	t_basesock \
	t_config_data \
	t_console \
	t_control \
	t_db \
	t_dgram \
	t_dgram_worker \
	t_game_obj \
	t_library \
	t_listensock \
	t_listensock_worker \
	t_log \
	t_motion_pool \
	t_octree \
	t_sockaddr \
	t_socket \
	t_stream \
	t_stream_worker \
	t_threadpool \
	t_update_pool \
	t_zone
endif
if WANT_PERL
  TC += t_perl
endif
if WANT_TCL
  TC += t_tcl t_tcl_exception
endif
if WANT_CLIENT
  TC += t_comm \
	t_comm_exception \
	t_configdata
endif

TESTS = $(TC)

check_PROGRAMS = $(TC)
EXTRA_DIST = t_texture_parser.xml

check: SUBDIRS = tap++ .

install-exec-local:
	@echo "Nothing to install"
	false

AM_CXXFLAGS = $(GMOCK_INCLUDES) $(GTEST_INCLUDES) $(TAP_INCLUDES)

CONFIG_DEFS = -DSERVER_ROOT_DIR=\"./\" -DSERVER_PID_FNAME=\"blah.pid\"
if HAVE_LIBWRAP
  CONFIG_DEFS += -DHAVE_LIBWRAP=1
endif

t_action_pool_SOURCES = t_action_pool.cc \
	../server/classes/action_pool.cc ../server/classes/action_pool.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_action_pool_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_action_pool_LDADD = $(GMOCK_MAIN_OBJ) $(GMOCK_LIBS) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_basesock_SOURCES = t_basesock.cc \
	../server/classes/basesock.cc ../server/classes/basesock.h \
	../server/classes/log.cc ../server/classes/log.h \
	../server/classes/config_data.cc ../server/classes/config_data.h
t_basesock_CXXFLAGS = $(CONFIG_DEFS) $(GTEST_INCLUDES)
t_basesock_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_config_data_SOURCES = t_config_data.cc \
	../server/classes/config_data.cc ../server/classes/config_data.h
t_config_data_CXXFLAGS = $(CONFIG_DEFS) $(TAP_INCLUDES)
t_config_data_LDADD = $(TAP_LDADD)

t_control_SOURCES = t_control.cc \
	../server/classes/control.cc ../server/classes/control.h
t_control_CXXFLAGS = $(CONFIG_DEFS) $(TAP_INCLUDES)
t_control_LDADD = $(TAP_LDADD) \
	../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_console_SOURCES = t_console.cc \
	../server/classes/modules/console.h \
	../server/classes/modules/console.cc \
	../server/classes/basesock.cc ../server/classes/basesock.h \
	../server/classes/log.cc ../server/classes/log.h \
	../server/classes/config_data.cc ../server/classes/config_data.h
t_console_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_console_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) $(WRAP_LDLIBS)

t_db_SOURCES = t_db.cc \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_db_LDADD = $(GTEST_LIBS) $(TAP_LDADD)

t_dgram_SOURCES = t_dgram.cc \
	../server/classes/dgram.cc ../server/classes/dgram.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_dgram_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_dgram_LDADD = $(GMOCK_MAIN_OBJ) $(GMOCK_LIBS) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_dgram_worker_SOURCES = t_dgram_worker.cc \
	../server/classes/dgram.cc ../server/classes/dgram.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_dgram_worker_CXXFLAGS = $(CONFIG_DEFS) $(GTEST_INCLUDES)
t_dgram_worker_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_game_obj_SOURCES = t_game_obj.cc \
	../server/classes/game_obj.cc ../server/classes/game_obj.h \
	../server/classes/control.cc ../server/classes/control.h \
	../server/classes/geometry.cc ../server/classes/geometry.h
t_game_obj_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_library_SOURCES = t_library.cc \
	../server/classes/library.cc ../server/classes/library.h
t_library_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) $(SERVER_LDLIBS)

t_listensock_SOURCES = t_listensock.cc \
	../server/classes/listensock.cc ../server/classes/listensock.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_listensock_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_listensock_LDADD = $(GMOCK_MAIN_OBJ) $(GMOCK_LIBS) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_listensock_worker_SOURCES = t_listensock_worker.cc \
	../server/classes/listensock.cc ../server/classes/listensock.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_listensock_worker_CXXFLAGS = $(CONFIG_DEFS) \
	$(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_listensock_worker_LDADD = $(GMOCK_MAIN_OBJ) $(GMOCK_LIBS) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_log_SOURCES = t_log.cc ../server/classes/log.cc ../server/classes/log.h \
	../server/classes/config_data.cc ../server/classes/config_data.h
t_log_CXXFLAGS = $(CONFIG_DEFS) $(TAP_INCLUDES)
t_log_LDADD = $(TAP_LDADD)

t_motion_pool_SOURCES = t_motion_pool.cc \
	../server/classes/motion_pool.cc ../server/classes/motion_pool.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_motion_pool_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_motion_pool_LDADD = $(GMOCK_MAIN_OBJ) $(GMOCK_LIBS) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_octree_SOURCES = t_octree.cc \
	../server/classes/octree.cc ../server/classes/octree.h
t_octree_CXXFLAGS = $(CONFIG_DEFS) $(GTEST_INCLUDES)
t_octree_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_sockaddr_SOURCES = t_sockaddr.cc ../server/classes/sockaddr.h
t_sockaddr_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_socket_SOURCES = t_socket.cc ../server/classes/socket.cc
t_socket_CXXFLAGS = $(CONFIG_DEFS) $(GTEST_INCLUDES)
t_socket_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) \
	../server/classes/libr9_classes.la $(SERVER_LDLIBS)

t_stream_SOURCES = t_stream.cc \
	../server/classes/stream.cc ../server/classes/stream.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_stream_CXXFLAGS = $(CONFIG_DEFS) $(GTEST_INCLUDES)
t_stream_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_stream_worker_SOURCES = t_stream_worker.cc \
	../server/classes/stream.cc ../server/classes/stream.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_stream_worker_CXXFLAGS = $(CONFIG_DEFS) $(GTEST_INCLUDES)
t_stream_worker_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_threadpool_SOURCES = t_threadpool.cc ../server/classes/thread_pool.h
t_threadpool_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS)

t_update_pool_SOURCES = t_update_pool.cc \
	../server/classes/update_pool.cc ../server/classes/update_pool.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_update_pool_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_update_pool_LDADD = $(GMOCK_MAIN_OBJ) $(GTEST_LIBS) $(GMOCK_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_zone_SOURCES = t_zone.cc \
	../server/classes/zone.cc ../server/classes/zone.h \
	../server/classes/modules/db.cc ../server/classes/modules/db.h
t_zone_CXXFLAGS = $(CONFIG_DEFS) $(GMOCK_INCLUDES) $(GTEST_INCLUDES)
t_zone_LDADD = $(GMOCK_MAIN_OBJ) $(GTEST_LIBS) $(GMOCK_LIBS) \
	../proto/libr9_proto.la ../server/classes/libr9_classes.la \
	$(SERVER_LDLIBS)

t_perl_SOURCES = t_perl.cc ../server/classes/modules/language.h \
	../server/classes/library.h ../server/classes/library.cc
t_perl_LDADD = $(GTEST_LIBS) $(SERVER_LDLIBS)

t_tcl_SOURCES = t_tcl.cc ../server/classes/modules/language.h \
	../server/classes/library.h ../server/classes/library.cc
t_tcl_LDADD = $(GTEST_LIBS) $(SERVER_LDLIBS)

t_tcl_exception_SOURCES = t_tcl_exception.cc \
	../server/classes/modules/tcl.cc ../server/classes/modules/r9tcl.h
t_tcl_exception_LDADD = $(GTEST_MAIN_OBJ) $(GTEST_LIBS) $(SERVER_LDLIBS) \
	$(TCL_LDLIBS)

t_comm_SOURCES = t_comm.cc ../client/comm.cc ../client/comm.h
t_comm_LDADD = $(GMOCK_MAIN_OBJ) $(GMOCK_LIBS) $(GTEST_LIBS) \
	../proto/libr9_proto.la

t_comm_exception_SOURCES = t_comm_exception.cc ../client/comm.cc \
	../client/comm.h
t_comm_exception_LDADD = $(TAP_LDADD)

t_configdata_SOURCES = t_configdata.cc \
	../client/configdata.h ../client/configdata.cc
t_configdata_LDADD = $(TAP_LDADD)

clean-local:
	rm -f *.gcno *.gcda *.gcov
	rm -f *.log *.trs
