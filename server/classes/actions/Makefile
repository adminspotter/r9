# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 28 Jun 2014, 09:44:24 tquirk
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
# This is the main makefile for the revision9 server action library.
#
# Changes
#   14 Aug 2006 TAQ - Created the file.
#   29 Sep 2007 TAQ - Updated target name.
#   20 Oct 2007 TAQ - Added -rdynamic to LIBFLAGS.  Renamed LIBFLAGS to the
#                     more universal LDFLAGS.
#   22 Oct 2007 TAQ - Added -D_REENTRANT to CFLAGS.
#   19 Sep 2013 TAQ - makedepend no longer lives in /usr/X11R6/bin.
#   10 May 2014 TAQ - Moved a bunch of defs into a top level Makefile.defs.
#   28 Jun 2014 TAQ - Switched over to libtool.
#
# Things to do
#

-include ../../../Makefile.defs

CXXFLAGS = -Wall -g -D_REENTRANT $(CINCLUDE)

LDPATHS =

HDRS = register.h
SRCS = register.cc control_object.cc move.cc
OBJS = ${SRCS:.cc=.lo}
TARGET = libr9_actions.la

.SUFFIXES : .cc .lo

.cc.lo :
	$(LIBTOOL) --mode=compile $(CXX) $(CXXFLAGS) -c $<

.PHONY : install uninstall clean tags all

all : Makefile.depend TAGS $(TARGET)

$(TARGET) : $(OBJS)
	$(LIBTOOL) --mode=link $(CXX) $(CXXFLAGS) -o $@ $(OBJS) -rpath $(LIBDIR) $(LDLIBS)

Makefile.depend : $(SRCS)
	@$(DEPEND) -- $(CFLAGS) -- $(SRCS) 2> /dev/null > Makefile.depend

tags : TAGS

TAGS : $(HDRS) $(SRCS)
	@$(MAKETAGS) $(HDRS) $(SRCS)

install : $(TARGET)
	$(LIBTOOL) --mode=install $(CP) $< $(LIBDIR)

uninstall :
	$(LIBTOOL) --mode=uninstall $(RM) $(LIBDIR)/$(TARGET)

clean:
	$(LIBTOOL) --mode=clean $(RM) $(OBJS) $(TARGET) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
