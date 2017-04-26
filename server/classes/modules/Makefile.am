modules_libs = libr9_console.la
if WANT_MYSQL
modules_libs += libr9_MySQL.la
endif
if WANT_PGSQL
modules_libs += libr9_pgsql.la
endif
if WANT_PYTHON
modules_libs += libr9_python.la
endif
if WANT_TCL
modules_libs += libr9_tcl.la
endif

lib_LTLIBRARIES = $(modules_libs)
AM_LDFLAGS = -module

libr9_console_la_SOURCES = console.cc console.h ../basesock.cc ../basesock.h \
	fdstreambuf.h
libr9_console_la_LIBADD = $(WRAP_LDLIBS)

libr9_MySQL_la_SOURCES = db.cc mysql_db.cc db.h
libr9_MySQL_la_LIBADD = $(MYSQL_LDLIBS)

libr9_pgsql_la_SOURCES = db.cc pgsql.cc db.h
libr9_pgsql_la_LIBADD = $(PGSQL_LDLIBS)

libr9_python_la_SOURCES = r9python.cc r9python.h language.cc language.h
libr9_python_la_CFLAGS = $(PYTHON_CFLAGS)
libr9_python_la_CXXFLAGS = $(PYTHON_CFLAGS)
libr9_python_la_LDFLAGS = -module $(PYTHON_LDFLAGS)

libr9_tcl_la_SOURCES = tcl.cc r9tcl.h language.cc language.h
libr9_tcl_la_LIBADD = $(TCL_LDLIBS)

clean-local:
	rm -f *.gcno *.gcda *.gcov
