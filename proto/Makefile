# Makefile
#   by Trinity Quirk <trinity@ymb.net>
#   last updated 12 May 2014, 21:32:15 tquirk
#
# Revision IX game server
# Copyright (C) 2007  Trinity Annabelle Quirk
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
# This is the main makefile for the revision9 protocol lib.
#
# Changes
#   26 Jul 2006 TAQ - Copied and modified from the client makefile.
#   02 Aug 2006 TAQ - Added tags creation.
#   29 Sep 2007 TAQ - Changed target name.
#   20 Oct 2007 TAQ - Renamed LIBFLAGS to LDFLAGS, added -rdynamic.
#   22 Oct 2007 TAQ - Added -D_REENTRANT to CDEFS.
#
# Things to do
#
# $Id: Makefile 10 2013-01-25 22:13:00Z trinity $

-include ../Makefile.defs

CDEBUG = -g
# COPTIM = -O2 -fno-strength-reduce
CINCLUDE =
CDEFS = -fPIC -DPIC -D_REENTRANT
CFLAGS = -Wall $(CDEFS) $(CDEBUG) $(COPTIM) $(CINCLUDE)

LDFLAGS = -shared -rdynamic
LDLIBS =

ARFLAGS = rcs

HDRS = proto.h
SRCS = byteswap.c
OBJS = ${SRCS:.c=.o}
TARGET = libr9_proto.so

all : Makefile.depend TAGS $(TARGET)

$(TARGET) : $(OBJS)
	$(LD) $(LDFLAGS) $(LDPATHS) -o $@ $(OBJS) $(LDLIBS)

Makefile.depend : $(SRCS)
	@$(DEPEND) -- $(CFLAGS) -- $(SRCS) 2> /dev/null > Makefile.depend

tags : TAGS

TAGS : $(HDRS) $(SRCS)
	@$(MAKETAGS) $(HDRS) $(SRCS)

clean :
	$(RM) $(OBJS) $(TARGET) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
