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
AM_LDFLAGS = -module

BUILT_SOURCES = perlxsi.c
CLEANFILES = perlxsi.c

perlxsi.c: Makefile
	perl -MExtUtils::Embed -e xsinit -- -o perlxsi.c

libr9_console_la_SOURCES = console.cc console.h ../basesock.cc ../basesock.h \
	fdstreambuf.h
libr9_console_la_LIBADD = $(WRAP_LDLIBS)

libr9_MySQL_la_SOURCES = db.cc mysql_db.cc db.h
libr9_MySQL_la_LIBADD = $(MYSQL_LDLIBS)

libr9_pgsql_la_SOURCES = db.cc pgsql.cc db.h
libr9_pgsql_la_LIBADD = $(PGSQL_LDLIBS)

libr9_perl_la_SOURCES = perl.cc r9perl.h perlxsi.c language.cc language.h
libr9_perl_la_CFLAGS = $(PERL_CFLAGS)
libr9_perl_la_CXXFLAGS = $(PERL_CFLAGS) -Wno-reserved-user-defined-literal
libr9_perl_la_LDFLAGS = $(PERL_LDFLAGS) -module

libr9_tcl_la_SOURCES = tcl.cc r9tcl.h language.cc language.h
libr9_tcl_la_LIBADD = $(TCL_LDLIBS)
