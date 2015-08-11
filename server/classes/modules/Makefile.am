modules_libs = libr9_console.la
if WANT_MYSQL
modules_libs += libr9_MySQL.la
endif
if WANT_PGSQL
modules_libs += libr9_pgsql.la
endif
if WANT_PERL
modules_libs += libr9_perl.la
endif
if WANT_TCL
modules_libs += libr9_tcl.la
endif

lib_LTLIBRARIES = $(modules_libs)

libr9_console_la_SOURCES = console.cc unix_console.cc inet_console.cc \
	console.h
libr9_console_la_CXXFLAGS = $(GXX_EXT_CXXFLAGS)

libr9_MySQL_la_SOURCES = db.cc mysql.cc db.h
libr9_MySQL_la_LDFLAGS = $(MYSQL_LDLIBS)

libr9_pgsql_la_SOURCES = db.cc pgsql.cc db.h
libr9_pgsql_la_LDFLAGS = $(PGSQL_LDLIBS)

libr9_perl_la_SOURCES = perl.cc r9perl.h language.h
libr9_perl_la_CXXFLAGS = $(PERL_CFLAGS)
libr9_perl_la_LDFLAGS = $(PERL_LDFLAGS)

libr9_tcl_la_SOURCES = tcl.cc r9tcl.h language.h
libr9_tcl_la_LDFLAGS = $(TCL_LDLIBS)
