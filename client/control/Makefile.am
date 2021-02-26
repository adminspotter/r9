clientlibdir = $(pkglibdir)/client

clientlib_LTLIBRARIES = keyboard.la

AM_LDFLAGS = -module -avoid-version $(CLIENT_LDFLAGS)

keyboard_la_SOURCES = keyboard.cc keyboard.h
keyboard_la_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES)
keyboard_la_LIBADD = -lcuddly_gl
