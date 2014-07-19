# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 19 Jul 2014, 08:55:43 tquirk
#
# Revision IX game server
# Copyright (C) 2014  Trinity Annabelle Quirk
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
#
# This is the makefile for the server modules (DB, embedded languages, etc.).
#
# Changes
#   13 Aug 2006 TAQ - Created the file.
#   12 Jun 2007 TAQ - Fixed perl includes and defines.  Added perl.o rule.
#                     Added tcl.o rule.  Added GPL notice.
#   08 Aug 2007 TAQ - Renamed OTHER(SRCS|OBJS) to LANG(SRCS|OBJS).
#   29 Sep 2007 TAQ - Updated to build shared libs instead of linked-in objs.
#   13 Oct 2007 TAQ - Since mysql.c became mysql.cc, we needed to update the
#                     rules a bit.
#   20 Oct 2007 TAQ - Added -rdynamic to LDFLAGS.
#   22 Oct 2007 TAQ - Added -D_REENTRANT to CDEFS.
#   22 Nov 2009 TAQ - Updated perl include to current version.
#   12 Sep 2013 TAQ - Again, updated perl version.
#   19 Sep 2013 TAQ - Genericized the TCL includes and libs.  We now build
#                     the pgsql lib by default.
#   10 May 2014 TAQ - Moved a bunch of defs into a top level file.
#   28 Jun 2014 TAQ - Moved over to libtool.  So much easier.  We've also
#                     got install/uninistall rules which will only operate
#                     on targets which have been built.
#   02 Jul 2014 TAQ - We're now linking libr9_console to the right libstdc++.
#                     Perl is now C++.
#   16 Jul 2014 TAQ - Made some changes for the perl and tcl embedded language
#                     stuff.  There's probably a more general way to do it.
#   19 Jul 2014 TAQ - Resolved merge conflicts for embedded languages.
#
# Things to do
#

-include ../../../Makefile.defs

-include Makefile.perldefs

CDEFS = -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -D_REENTRANT
CFLAGS = -Wall -g $(CDEFS) $(CINCLUDE)
CON_CFLAGS = $(CFLAGS) -I/usr/include/c++/4.2.1
MY_CFLAGS = $(CFLAGS) -I/usr/local/mysql/include
TCL_CFLAGS = $(CFLAGS) -I/usr/include/tcl

#LIBWRAP = -lwrap
LIBMYSQL = -L/usr/local/mysql/lib -lmysqlclient_r
LIBPQ = -lpq
LIBTCL = -ltcl
LIBSTDCPP = -lstdc++.6

HDRS = db.h console.h unix_console.h inet_console.h mysql.h pgsql.h perl.h tcl.h
MY_SRCS = db.cc mysql.cc
PQ_SRCS = db.cc pgsql.cc
CON_SRCS = console.cc unix_console.cc inet_console.cc
LANG_SRCS = perl.cc tcl.cc
CXX_SRCS = $(MY_SRCS) $(PQ_SRCS) $(CON_SRCS) $(LANG_SRCS)
MY_OBJS = ${MY_SRCS:.cc=.lo}
PQ_OBJS = ${PQ_SRCS:.cc=.lo}
CON_OBJS = ${CON_SRCS:.cc=.lo}
LANG_OBJS = ${LANG_SRCS:.cc=.lo}
TARGETS = libr9_console.la libr9_MySQL.la libr9_pgsql.la libr9_perl.la \
	libr9_tcl.la

.SUFFIXES : .cc .lo

.cc.lo :
	$(LIBTOOL) --mode=compile $(CXX) $(CFLAGS) -c $<

.PHONY : install uninstall clean tags all

all : Makefile.depend TAGS $(TARGETS)

console.lo : console.cc
	$(LIBTOOL) --mode=compile $(CXX) $(CON_CFLAGS) -c $<

unix_console.lo : unix_console.cc
	$(LIBTOOL) --mode=compile $(CXX) $(CON_CFLAGS) -c $<

inet_console.lo : inet_console.cc
	$(LIBTOOL) --mode=compile $(CXX) $(CON_CFLAGS) -c $<

libr9_console.la : $(CON_OBJS)
	$(LIBTOOL) --mode=link $(CXX) $(CON_CFLAGS) -o $@ $(CON_OBJS) -rpath $(LIBDIR) $(LIBWRAP) $(LIBSTDCPP)

mysql.lo : mysql.cc
	$(LIBTOOL --mode=compile $(CXX) $(MY_CFLAGS) -c $<

libr9_MySQL.la : $(MY_OBJS)
	$(LIBTOOL) --mode=link $(CXX) $(MY_CFLAGS) -o $@ $(MY_OBJS) -rpath $(LIBDIR) $(LIBMYSQL)

libr9_pgsql.la : $(PQ_OBJS)
	$(LIBTOOL) --mode=link $(CXX) $(CFLAGS) -o $@ $(PQ_OBJS) -rpath $(LIBDIR) $(LIBPQ)

perl.lo : perl.cc
	$(LIBTOOL) --mode=compile $(CXX) $(PERL_CFLAGS) -c $<

libr9_perl.la : perl.lo
	$(LIBTOOL) --mode=link $(CXX) $(PERL_CFLAGS) -o $@ $< -rpath $(LIBDIR) $(PERL_LDFLAGS)

tcl.lo : tcl.cc
	$(LIBTOOL) --mode=compile $(CXX) $(TCL_CFLAGS) -c $<

libr9_tcl.la : tcl.lo
	$(LIBTOOL) --mode=link $(CC) $(TCL_CFLAGS) -o $@ $< -rpath $(LIBDIR) $(LIBTCL)

Makefile.depend : $(CXXSRCS)
	@$(DEPEND) -- $(CFLAGS) -- $(CXXSRCS) 2> /dev/null > Makefile.depend

Makefile.perldefs :
	echo "PERL_CFLAGS = `perl -MExtUtils::Embed -e ccopts`" > $@
	echo "PERL_LDFLAGS = `perl -MExtUtils::Embed -e ldopts`" >> $@

tags : TAGS

TAGS : $(HDRS) $(CXXSRCS)
	@$(MAKETAGS) $(HDRS) $(CXXSRCS)

# Only install the targets which have been built
install :
	for file in $(TARGETS) ; do if [ -e $$file ] ; then echo $$file ; $(LIBTOOL) --mode=install $(CP) $$file $(LIBDIR) ; fi ; done

# Only uninstall the targets which have been built
uninstall :
	for file in $(TARGETS) ; do if [ -e $$file ] ; then $(LIBTOOL) --mode=uninstall $(RM) $(LIBDIR)/$$file ; fi ; done

clean :
	$(LIBTOOL) --mode=clean $(RM) $(MY_OBJS) $(PQ_OBJS) $(CON_OBJS) $(LANG_OBJS) $(TARGETS) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
