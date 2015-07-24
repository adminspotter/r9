# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 24 Jul 2015, 13:26:01 tquirk
#
# Revision IX game server
# Copyright (C) 2015  Trinity Annabelle Quirk
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
# This is the main makefile for the revision9 project.
#
# Changes
#   26 Jul 2006 TAQ - Copied and modified from the client makefile.
#   02 Aug 2006 TAQ - Added tags creation.  Added archive creation.
#   24 Jul 2015 TAQ - Comment cleanup.
#
# Things to do
#

SHELL = /bin/bash

CC = /usr/bin/gcc
CDEBUG = -g
# COPTIM = -O2 -fno-strength-reduce
CINCLUDE =
CDEFS =
CFLAGS = -Wall $(CDEFS) $(CDEBUG) $(COPTIM) $(CINCLUDE)

LD = /usr/bin/gcc
LDFLAGS =
LDLIBS =

DEPEND = /usr/X11R6/bin/makedepend -f-

RM = /bin/rm -f

AR = /usr/bin/ar
ARFLAGS = rcs

TAR = /bin/tar
TARFLAGS = czf

HDRS =
SRCS =
OBJS = ${SRCS:.c=.o}
TARGET =

.PHONY : all clean

all :
	$(MAKE) -C proto
	$(MAKE) -C server
	$(MAKE) -C client

archive : clean
	@(cd .. ; $(TAR) $(TARFLAGS) revision9.tar.gz revision9)

Makefile.depend : $(SRCS)
	@$(DEPEND) -- $(CFLAGS) -- $(SRCS) 2> /dev/null > Makefile.depend

tags :
	$(MAKE) -C proto tags
	$(MAKE) -C server tags
	$(MAKE) -C client tags

clean :
	$(MAKE) -C proto clean
	$(MAKE) -C server clean
	$(MAKE) -C client clean
	$(RM) $(OBJS) $(TARGET) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
