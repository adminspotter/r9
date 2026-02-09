client_libs = keyboard.la joystick.la spacemouse.la

clientlibdir = $(pkglibdir)/client

clientlib_LTLIBRARIES = $(client_libs)

AM_LDFLAGS = -module -avoid-version $(CLIENT_LDFLAGS)

keyboard_la_SOURCES = keyboard.cc keyboard.h
keyboard_la_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES)
keyboard_la_LIBADD = -lcuddly_gl

joystick_la_SOURCES = joystick.cc joystick.h
joystick_la_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES)
joystick_la_LIBADD = -lcuddly_gl

spacemouse_la_SOURCES = spacemouse.cc spacemouse.h
spacemouse_la_CXXFLAGS = $(CLIENT_CXXFLAGS) $(CLIENT_INCLUDES)
spacemouse_la_LIBADD = -lcuddly_gl

install-data-hook:
	cd $(clientlibdir) && rm -f $(client_libs)
