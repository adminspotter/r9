# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 12 Sep 2013, 13:26:24 trinity
#
# This is the main makefile for the revision9 project.
#
# Changes
#   26 Jul 2006 TAQ - Copied and modified from the client makefile.
#   02 Aug 2006 TAQ - Added tags creation.  Added archive creation.
#
# Things to do
#
# $Id: Makefile 3 2007-09-29 04:16:16Z wwagner $

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
