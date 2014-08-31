# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 30 Aug 2014, 09:20:30 tquirk
#
# Revision IX game client
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
# This is the main makefile for the revision9 client.
#
# Changes
#   26 Sep 1998 TAQ - Created the file.
#   18 Jul 2006 TAQ - Fixed the heck out of it.
#   26 Jul 2006 TAQ - Made the protocol lib an actual lib on the link line,
#                     instead of an object to be built.
#   02 Aug 2006 TAQ - Added TAGS creation.  Moved the R9 libs to ~/lib.
#   03 Aug 2006 TAQ - Added a couple new files, geometry.c and texture.c.
#   30 Sep 2007 TAQ - Updated protocol lib name.
#   15 Dec 2007 TAQ - $TARGET depends on the proto lib.
#   12 Sep 2013 TAQ - makedepend no longer lives in /usr/X11R6/bin.
#   26 Jul 2014 TAQ - C files are becoming C++.
#   30 Aug 2014 TAQ - We're not linking anything here, so removed all the
#                     library stuff.
#
# Things to do
#

-include ../Makefile.defs

CDEBUG = -g
# COPTIM = -O2 -fno-strength-reduce
CINCLUDE = -I/usr/local/include -I/usr/X11R6/include -I/home/trinity/include -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/GLUT.framework/Headers
CDEFS = -DDO_DEBUG
CFLAGS = -Wall $(CDEFS) $(CDEBUG) $(COPTIM) $(CINCLUDE)

HDRS = cache.h geometry.h texture.h comm.h config.h
CXXSRCS = geometry.cc texture.cc comm.cc config.cc
OBJS = ${CXXSRCS:.cc=.o}

.SUFFIXES : .cc .o

.cc.o :
	$(CXX) $(CFLAGS) -c $< -o $@

.PHONY : clean

all : Makefile.depend TAGS $(OBJS)

Makefile.depend : $(SRCS) $(CXXSRCS)
	@$(DEPEND) -- $(CFLAGS) -- $(SRCS) $(CXXSRCS) 2> /dev/null > Makefile.depend

tags : TAGS

TAGS : $(HDRS) $(SRCS)
	@$(MAKETAGS) $(HDRS) $(SRCS)

clean :
	$(RM) $(OBJS) $(TARGET) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
