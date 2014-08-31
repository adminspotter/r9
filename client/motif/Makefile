# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 31 Aug 2014, 16:24:35 tquirk
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
# This is the main makefile for the revision9 Motif client.
#
# Changes
#   30 Aug 2014 TAQ - Created the file
#   31 Aug 2014 TAQ - Should be good enough to get things linked, once we
#                     can get all the symbols fixed up.
#
# Things to do
#

-include ../../Makefile.defs

CINCLUDE = -I/opt/X11/include -I/usr/local/include -I$(HOME)/src/Eigen -I/System/Library/Frameworks/OpenGL.framework/Headers -I$(HOME)/include
CFLAGS = -Wall $(CINCLUDE)

LDFLAGS = -L/opt/X11/lib -L/System/Library/Frameworks/OpenGL.framework/Libraries -L$(HOME)/lib
XMLIBS = -lXm
GLLIBS = -lGLw -lglut -lGLU -lGL
XLIBS = $(XMLIBS) -lXext -lXt -lX11 -lXmu -lSM -lICE -lXpm
LDLIBS = $(GLLIBS) $(XLIBS) -lxerces-c -lr9_proto -lm

HDRS = client.h msglog.h
CSRCS = about.c command.c
CXXSRCS = client.cc menu.cc message.cc msglog.cc setup.cc view.cc
OBJS = ${CSRCS:.c=.o} ${CXXSRCS:.cc=.o}
COMMONOBJS = ../comm.o ../config.o ../geometry.o ../texture.o
TARGET = client

.SUFFIXES : .c .cc .o

.c.o :
	$(CC) $(CFLAGS) -c $< -o $@

.cc.o :
	$(CXX) $(CFLAGS) -c $< -o $@

.PHONY : clean

all : Makefile.depend TAGS $(TARGET)

$(TARGET) : $(OBJS) $(COMMONOBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(COMMONOBJS) $(LDLIBS)

Makefile.depend : $(CSRCS) $(CXXSRCS)
	@$(DEPEND) -- $(CFLAGS) -- $(CSRCS) $(CXXSRCS) 2> /dev/null > Makefile.depend

tags : TAGS

TAGS : $(HDRS) $(CSRCS) $(CXXSRCS)
	@$(MAKETAGS) $(HDRS) $(CSRCS) $(CXXSRCS)

clean :
	$(RM) $(OBJS) $(TARGET) core TAGS Makefile.bak Makefile.depend
