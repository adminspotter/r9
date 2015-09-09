GMOCK_DIR = gmock-1.7.0

SUBDIRS = $(GMOCK_DIR)

GTEST_LIBS = $(GMOCK_DIR)/gtest/lib/libgtest.la
GMOCK_LIBS = $(GMOCK_DIR)/lib/libgmock.la $(GTEST_LIBS)

TC = t_library t_log
if WANT_CLIENT
  TC += t_cache
endif

TESTS = $(TC)

check_PROGRAMS = $(TC)
noinst_LTLIBRARIES = libtest.la

AM_CXXFLAGS = -I$(GMOCK_DIR)/gtest/include -I$(GMOCK_DIR)/include

t_library_SOURCES = t_library.cc tap.h tap_main.cc \
	../server/classes/library.cc ../server/classes/library.h
t_library_LDADD = $(GTEST_LIBS)

t_log_SOURCES = t_log.cc tap.h \
	../server/log.cc ../server/log.h
t_log_LDADD = $(GTEST_LIBS)

t_cache_SOURCES = t_cache.cc tap.h tap_main.cc ../client/cache.h
t_cache_LDADD = $(GTEST_LIBS) $(GMOCK_LIBS) $(CLIENT_LDLIBS)

libtest_la_SOURCES = libtest.cc libtest.h
libtest_la_LDFLAGS = -rpath /nowhere
