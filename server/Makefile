# Makefile
#   by Trinity Quirk <trinity@ymb.net>
#   last updated 21 Jun 2014, 17:35:28 tquirk
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
#
# Things to do
#

-include ../Makefile.defs

CXXFLAGS = -Wall -g -fPIC -DPIC -D_REENTRANT $(CINCLUDE)

LDFLAGS = -rdynamic -L/usr/local/lib -L/home/trinity/lib
LDLIBS = -lr9_console -lr9_classes -lr9_proto -lmath3d -lm -ldl -lpthread -lstdc++

DEPEND = $(MAKEDEPEND) -f-

HDRS = config.h server.h signals.h subserver.h
CXXSRCS = config.cc server.cc signals.cc subserver.cc
OBJS = ${CXXSRCS:.cc=.o}
TARGET = server

.PHONY : clean proto classes

all : proto classes Makefile.depend TAGS $(TARGET)

proto :
	$(MAKE) -C ../proto

classes :
	$(MAKE) -C classes

$(TARGET) : $(OBJS) classes/libr9_classes.so ../proto/libr9_proto.so
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

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
