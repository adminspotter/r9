# Makefile
#   by Trinity Quirk <trinity@ymb.net>
#   last updated 23 Jun 2014, 18:32:01 tquirk
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
#
# Things to do
#

-include ../Makefile.defs

CXXFLAGS = -Wall -g -fPIC -DPIC -D_REENTRANT $(CINCLUDE)

LDFLAGS = -rdynamic -L/usr/local/lib -L$(HOME)/lib -Lclasses/modules
LDLIBS = -lr9_proto -lm -ldl -lpthread -lstdc++

DEPEND = $(MAKEDEPEND) -f-

HDRS = config.h server.h signals.h subserver.h log.h
CXXSRCS = config.cc server.cc signals.cc subserver.cc log.cc
OBJS = ${CXXSRCS:.cc=.o}
TARGET = server

.PHONY : clean proto classes

all : proto classes Makefile.depend TAGS $(TARGET)

proto :
	$(MAKE) -C ../proto

classes :
	$(MAKE) -C classes

$(TARGET) : $(OBJS) classes/r9_classes.a classes/modules/r9_console.a ../proto/libr9_proto.so
	$(LD) $(LDFLAGS) -o $@ $(OBJS) classes/r9_classes.a classes/modules/r9_console.a $(LDLIBS)

Makefile.depend : $(SRCS) $(HDRS)
	@$(DEPEND) -- $(CFLAGS) -- $(SRCS) 2> /dev/null > Makefile.depend

tags : TAGS
	$(MAKE) -C classes tags

TAGS : $(HDRS) $(SRCS)
	@$(MAKETAGS) $(HDRS) $(SRCS)

clean :
	$(MAKE) -C classes clean
	$(RM) $(OBJS) $(TARGET) core TAGS Makefile.bak Makefile.depend

-include Makefile.depend
