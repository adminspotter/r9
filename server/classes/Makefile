# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 31 May 2014, 11:01:22 tquirk
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
# This is the main makefile for the revision9 server C++ classes.
#
# Changes
#   02 Aug 2006 TAQ - Added comment block.  Added TAGS creation.  Moved the
#                     R9 libs to ~/lib.
#   13 Aug 2006 TAQ - Added modules subdir.
#   14 Aug 2006 TAQ - Made $(TARGET) rule use $(TARGET) in the predicate.
#                     Removed duplicated AR and ARFLAGS definitions.
#   16 Aug 2006 TAQ - Removed test targets, since the files have moved.
#   12 Jun 2007 TAQ - Added targets for actions subdir.
#   09 Sep 2007 TAQ - Added dgram and stream targets.
#   13 Sep 2007 TAQ - Added basesock target.
#   29 Sep 2007 TAQ - Updated some target names.
#   13 Oct 2007 TAQ - Slight updates.
#   20 Oct 2007 TAQ - Added -rdynamic to LDFLAGS.
#   22 Oct 2007 TAQ - Added -D_REENTRANT to CXXFLAGS.
#   15 Dec 2007 TAQ - The modules dir is not a dependency for $TARGET.
#   19 Sep 2013 TAQ - makedepend is no longer in /usr/X11R6/bin.
#   10 May 2014 TAQ - motion.cc changed names, and we added a motion class.
#                     Moved a bunch of defs into a top level Makefile.defs.
#
# Things to do
#

-include ../../Makefile.defs

CXXFLAGS = -Wall -g -fPIC -DPIC -D_REENTRANT $(CINCLUDE)

LFLAGS =

YFLAGS = -y -d -v

LDPATHS = -L/usr/local/lib -L/Users/tquirk/lib
LDLIBS = -lr9_proto -lpthread
LDFLAGS = -shared -rdynamic
EXTRALIBS = -ldl

DEPEND = $(MAKEDEPEND) -f-

ARFLAGS = rcs

HDRS = control.h defs.h game_obj.h geometry.h motion.h \
	thread_pool.h zone.h zone_interface.h basesock.h dgram.h stream.h
SRCS = control.cc game_obj.cc geometry.cc zone.cc access.cc action.cc \
	motion.cc motion_worker.cc update.cc send.cc zone_interface.cc \
	basesock.cc dgram.cc stream.cc
OBJS = ${SRCS:.cc=.o}
TARGET = libr9_classes.so

.SUFFIXES : .y .cc .o

.cc.o :
	$(CXX) $(CXXFLAGS) -c $< -o $@

#.PHONY : actions modules clean tags all
.PHONY : actions clean tags all

#all : actions modules Makefile.depend TAGS $(TARGET)
all : Makefile.depend TAGS $(TARGET)

actions :
	$(MAKE) -C actions

#modules :
#	$(MAKE) -C modules

$(TARGET) : $(OBJS)
	$(LD) $(LDFLAGS) $(LDPATHS) -o $(TARGET) $(OBJS) $(LDLIBS)

Makefile.depend : $(SRCS)
	@$(DEPEND) -- $(CFLAGS) -- $(SRCS) 2> /dev/null > Makefile.depend

tags : TAGS
	$(MAKE) -C actions tags
#	$(MAKE) -C modules tags

TAGS : $(HDRS) $(SRCS)
	@$(MAKETAGS) $(HDRS) $(SRCS)

clean:
	$(MAKE) -C actions clean
#	$(MAKE) -C modules clean
	$(RM) $(OBJS) $(TARGET) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
