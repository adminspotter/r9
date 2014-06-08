# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 08 Jun 2014, 15:02:41 tquirk
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
#
# Things to do
#
# $Id$

-include ../../../Makefile.defs

CPERLINC = -I/usr/lib/perl/5.14/CORE
CTCLINC = -I/usr/include/tcl
CMOREINCLUDE = -I.. -I../.. $(CPERLINC) $(CTCLINC)
CDEBUG = -g
# COPTIM = -O2
CDEFS = -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -D_REENTRANT
CFLAGS = -Wall -fPIC -DPIC $(CDEFS) $(CDEBUG) $(COPTIM) $(CINCLUDE) $(CMOREINCLUDE)

LFLAGS =

YFLAGS = -y -d -v

#LIBWRAP = -lwrap
LIBMYSQL = -lmysqlclient_r
LIBPQ = -lpq
LIBPERL = -lperl
LIBTCL = -ltcl
LDFLAGS = -shared -rdynamic -L/usr/local/lib -L$(HOME)/lib
LDLIBS =

DEPEND = $(MAKEDEPEND) -f-

ARFLAGS = rcs

HDRS = db.h console.h mysql.h pgsql.h
CXXSRCS = db.cc mysql.cc pgsql.cc console.cc unix_console.cc inet_console.cc
SRCS = perl.c tcl.c
OBJS = ${CXXSRCS:.cc=.o} ${SRCS:.c=.o}
TARGETS = libr9_console.so libr9_mysql.so libr9_pgsql.so libr9_perl.so \
	libr9_tcl.so

.SUFFIXES : .cc .o

.cc.o :
	$(CXX) $(CFLAGS) -c $< -o $@

.PHONY : clean tags all

all : Makefile.depend TAGS $(TARGETS)

libr9_console.so : console.o unix_console.o inet_console.o
	$(CXX) $(LDFLAGS) $(LIBWRAP) $< -o $@

libr9_mysql.so : mysql.o
	$(CXX) $(LDFLAGS) $(LIBMYSQL) $< -o $@

libr9_pgsql.so : pgsql.o
	$(CXX) $(LDFLAGS) $(LIBPQ) $< -o $@

libr9_perl.so : perl.o
	$(LD) $(LDFLAGS) $(LIBPERL) $< -o $@

libr9_tcl.so : tcl.o
	$(LD) $(LDFLAGS) $(LIBTCL) $< -o $@

Makefile.depend : $(SRCS)
	@$(DEPEND) -- $(CFLAGS) -- $(SRCS) $(CXXSRCS) 2> /dev/null > Makefile.depend

tags : TAGS

TAGS : $(HDRS) $(CXXSRCS) $(SRCS)
	@$(MAKETAGS) $(HDRS) $(CXXSRCS) $(SRCS)

clean :
	$(RM) $(OBJS) $(TARGETS) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
