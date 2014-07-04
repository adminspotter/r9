# Makefile
#   by Trinity Quirk <tquirk@ymb.net>
#   last updated 04 Jul 2014, 16:59:49 tquirk
#
# This is the main makefile for the revision9 server.
#
# Changes
#   02 Aug 2006 TAQ - Added this comment block.  Added TAGS creation.
#                     Moved the R9 libs to ~/lib.
#   13 Aug 2006 TAQ - Fixed some tiny stuff.
#   14 Aug 2006 TAQ - Added lib for mysql stuff.
#   13 Sep 2007 TAQ - Removed sockets.c, tcpserver.c, subserver.c, and
#                     udpserver.c, since they're now being done in some
#                     C++ classes.
#   30 Sep 2007 TAQ - Updated lib names and lib requirements, since the
#                     libs which are loaded work differently.  Updated
#                     targets to include libr9_proto.so.
#   22 Oct 2007 TAQ - Added -D_REENTRANT to CDEFS.
#   15 Dec 2007 TAQ - $TARGET is dependent on the classes and proto libs too.
#   12 Sep 2013 TAQ - makedepend is no longer in /usr/X11R6/bin.
#   19 Sep 2013 TAQ - We now link against libr9_console.so.
#   21 Jun 2014 TAQ - Moving the main stuff to C++.  Also modified to include
#                     our general Makefile.defs file.
#   23 Jun 2014 TAQ - Working on getting the binary to link properly.
#   29 Jun 2014 TAQ - The final part of the libtool-ification.
#   02 Jul 2014 TAQ - Finally got the binary to link to the right stdc++ lib.
#   04 Jul 2014 TAQ - The subserver is now a separate binary.
#
# Things to do
#

-include ../Makefile.defs

CXXFLAGS = -Wall -g -fPIC -DPIC -D_REENTRANT $(CINCLUDE)

LDLIBS = -lpthread

HDRS = config.h server.h signals.h subserver.h log.h
CXXSRCS = config.cc server.cc signals.cc log.cc
SUBSRV_SRCS = subserver.cc log.cc
OBJS = ${CXXSRCS:.cc=.o}
SUBSRV_OBJS = ${SUBSRV_SRCS:.cc=.o}
TARGET = server
SUBSRV_TARGET = subserver

.PHONY : install uninstall clean

all : Makefile.depend TAGS $(TARGET) $(SUBSRV_TARGET)

../proto/libr9_proto.la :
	$(MAKE) -C ../proto

classes/libr9_classes.la :
	$(MAKE) -C classes

classes/modules/libr9_console.la :
	$(MAKE) -C classes/modules libr9_console.la

$(TARGET) : ../proto/libr9_proto.la classes/libr9_classes.la classes/modules/libr9_console.la $(OBJS)
	$(LIBTOOL) --mode=link $(CXX) $(CXXFLAGS) -o $@ $(OBJS) classes/libr9_classes.la classes/modules/libr9_console.la ../proto/libr9_proto.la $(LDLIBS)

$(SUBSRV_TARGET) : $(SUBSRV_OBJS)
	$(LIBTOOL) --mode=link $(CXX) $(CXXFLAGS) -o $@ $(SUBSRV_OBJS)

Makefile.depend : $(SRCS) $(HDRS)
	@$(DEPEND) -- $(CFLAGS) -- $(SRCS) 2> /dev/null > Makefile.depend

tags : TAGS
	$(MAKE) -C classes tags

TAGS : $(HDRS) $(SRCS)
	@$(MAKETAGS) $(HDRS) $(SRCS)

install :
	$(MAKE) -C ../proto install
	$(MAKE) -C classes install
	$(LIBTOOL) --mode=install $(CP) $(TARGET) $(HOME)/bin
	$(LIBTOOL) --mode=install $(CP) $(SUBSRV_TARGET) $(HOME)/bin

uninstall :
	$(MAKE) -C ../proto uninstall
	$(MAKE) -C classes uninstall
	$(RM) $(HOME)/bin/$(TARGET)
	$(RM) $(HOME)/bin/$(SUBSRV_TARGET)

clean :
	$(MAKE) -C ../proto clean
	$(MAKE) -C classes clean
	$(LIBTOOL) --mode=clean $(RM) $(TARGET)
	$(LIBTOOL) --mode=clean $(RM) $(SUBSRV_TARGET)
	$(RM) $(OBJS) $(SUBSRV_OBJS) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
